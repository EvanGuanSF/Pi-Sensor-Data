from gpiozero import DistanceSensor
from gpiozero.pins.pigpio import PiGPIOFactory
import math
import time
import datetime
from pathlib import Path
import fcntl

from board import SCL, SDA
import busio
from PIL import Image, ImageDraw, ImageFont
import adafruit_ssd1306

# Create the I2C interface.
i2c = busio.I2C(SCL, SDA)

# Create the SSD1306 OLED class.
# The first two parameters are the pixel width and pixel height.  Change these
# to the right size for your display!
disp = adafruit_ssd1306.SSD1306_I2C(128, 64, i2c)

# Clear display.
disp.fill(0)
disp.show()

# Create blank image for drawing.
# Make sure to create image with mode '1' for 1-bit color.
width = disp.width
height = disp.height
image = Image.new("1", (width, height))

# Get drawing object to draw on image.
draw = ImageDraw.Draw(image)

# Draw a black filled box to clear the image.
draw.rectangle((0, 0, width, height), outline=0, fill=0)

# Draw some shapes.
# First define some constants to allow easy resizing of shapes.
padding = -2
top = padding
bottom = height - padding
# Move left to right keeping track of the current x position for drawing shapes.
x = 0

# Load the fonts to be used.
big_font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 28)
sml_font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16)

# Status dot to indicate visually if the program is running correctly.
activity_dot_toggle = True
activity_dot_size = 3

factory = PiGPIOFactory()

trig_pin = 23
echo_pin = 24
dist_sensor = DistanceSensor(echo = echo_pin, trigger = trig_pin, pin_factory = factory)
seconds_to_activate = 5
run_until_time = 0
distance_cm = 0.0
time_between_samples = 0.05
showing = False

while True:
    distance_cm = dist_sensor.distance * 100
    print('Distance: {0}cm'.format(distance_cm))
    if distance_cm <= 10.0:
        run_until_time = math.floor(time.time() * 1000) + seconds_to_activate * 1000
        showing = True
    if not showing:
        # Draw a black filled box to clear the image.
        draw.rectangle((0, 0, width, height), outline=0, fill=0)
        disp.image(image)
        disp.show()

    if showing and math.floor(time.time() * 1000) <= run_until_time:
        # Draw a black filled box to clear the image.
        draw.rectangle((0, 0, width, height), outline=0, fill=0)

        # Possible race condition between reading and writing to/from the data file.
        # Try to get a lock on the data file and then read data.
        data_file_name = str(Path.home()) + '/.pi_sensor_data/bme680_data.txt'
        # Try to read data until successful.
        data_read = False
        while (data_read is False):
            with open(data_file_name, 'r+') as data_file:
                fcntl.flock(data_file, fcntl.LOCK_EX)

                try:
                    temp = float(data_file.readline().rstrip().split(':')[1])
                    hum = float(data_file.readline().rstrip().split(':')[1])
                except:
                    print(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S") + ': File read error occured, trying again...')
                    time.sleep(0.1)
                else:
                    data_read = True

                fcntl.flock(data_file, fcntl.LOCK_UN)
                data_file.close()

        # Draw program activity status dot
        if activity_dot_toggle:
            draw.rectangle((width - activity_dot_size, height - activity_dot_size, width, height), outline=255, fill=255)
            activity_dot_toggle = False
        else:
            draw.rectangle((width - activity_dot_size, height - activity_dot_size, width, height), outline=0, fill=0)
            activity_dot_toggle = True

        # Write two lines of text to the buffer.
        draw.text((x + 1, top + 1), str('{0:.1f}'.format(temp * 1.8 + 32)) + "°F", font=big_font, fill=255)
        draw.text((x + 1, top + 1 + 30), str('{0:.1f}'.format(temp)) + "°C", font=sml_font, fill=255)
        draw.text((x + 1, top + 1 + 30 + 18), str('{0:.1f}'.format(hum)) + "%RH", font=sml_font, fill=255)

        # Display image buffer.
        disp.image(image)
        disp.show()
        time.sleep(1 - time_between_samples)
    if showing and math.floor(time.time() * 1000) > run_until_time:
        showing = False

    time.sleep(time_between_samples)
