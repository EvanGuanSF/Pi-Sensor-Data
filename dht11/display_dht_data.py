import time
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

while True:
    # Do a wait at the start of the loop in case 
    time.sleep(1)

    # Draw a black filled box to clear the image.
    draw.rectangle((0, 0, width, height), outline=0, fill=0)

    # Possible race condition between reading and writing to/from the data file.
    # Try to get a lock on the data file and then read data.
    data_file_name = str(Path.home()) + '/.pi_sensor_data/dht_data.txt'
    with open(data_file_name, 'r+') as data_file:
        fcntl.flock(data_file, fcntl.LOCK_EX)

        # Try to read data until successful.
        data_read = False
        while (data_read is False):
            try:
                temp = float(data_file.readline().rstrip().split(':')[1])
                hum = float(data_file.readline().rstrip().split(':')[1])
            except:
                print(time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime()) + ': File read error occured, trying again...')
                time.sleep(0.01)
            else:
                data_read = True

        fcntl.flock(data_file, fcntl.LOCK_UN)
        data_file.close()

    # Write two lines of text to the buffer.
    draw.text((x + 1, top + 1), str('{0:.1f}'.format(temp * 1.8 + 32)) + "°F", font=big_font, fill=255)
    draw.text((x + 1, top + 1 + 30), str('{0:.1f}'.format(temp)) + "°C", font=sml_font, fill=255)
    draw.text((x + 1, top + 1 + 30 + 18), str('{0:.1f}'.format(hum)) + "%RH", font=sml_font, fill=255)

    # Display image buffer.
    disp.image(image)
    disp.show()
