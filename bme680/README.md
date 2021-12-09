This program reads sensor data from a Bosch Sensortec BME680 module on a Raspberry Pi via i2c and sends the resulting data to a user specified MongoDB server.

### Requirements:<br>
```sudo apt-get install -y python3 crontab gcc pkg-config```
- python to run the sensor reading program
- crontab for timed jobs
- gcc to build the data sending program
- pkg-config for generating gcc parameters

Pimeroni BME680 sensor library to access the sensor.
Get it here: ```https://github.com/pimoroni/bme680```

(Optional) Adafruit SSD1306 library to address a small (128x64) i2c oled display.
Get it here: ```https://github.com/adafruit/Adafruit_CircuitPython_SSD1306```

We need the mongoc driver to access MongoDB databases. Build and install it as per:<br>
```http://mongoc.org/libmongoc/current/installing.html#build-environment-on-unix```<br>

If you wish to build and utilize a MongoDB server on your Raspberry Pi 4 (64 bit OS required), I have a written guide here:<br>
https://gist.github.com/JammyPajamies/9f06f00c1732c582c9714c603fb58f4b

---
### Installation:
Build the program, configure the run scripts, and add a crontab job for run script execution:<br>
```./install.sh -u mongodb_username -p mongodb-password -h mongodb_hostname -d database_name -c collection_name -n n_minutes_per_datapoint```<br>
The program should now attempt to get sensor data every second from the BME680 sensor and should try to upload data every n_minutes_per_datapoint minutes to your mongodb server.
---
To check if the upload cronjob is active, run:
```crontab -l```