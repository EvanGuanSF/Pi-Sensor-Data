This program reads sensor data from a Bosch Sensortec BME680 module on a Raspberry Pi via i2c and sends the resulting data to a user specified MongoDB server.

Multiple reporting programs could potentially fight over access to the sensor, so data is made available by a dedicated sensor data reading program to other programs on a shared memory map at `/bme680_data`. Cronjobs are used for minute resolution reporting intervals and looping/sleeping shell scripts are used for sub-minute intervals. 

Both reading and reporting are run at startup via combination of systemd services and cronjobs.

### Requirements:<br>
```sudo apt-get install -y python3 crontab gcc pkg-config```
- python to run the sensor reading program
- crontab for timed jobs
- gcc to build the data sending program
- pkg-config for generating gcc parameters

Pimeroni BME680 sensor library to access the sensor.
Get it here: ```https://github.com/pimoroni/bme680```


We need the mongoc driver to access MongoDB databases.
Build and install it as per:```http://mongoc.org/libmongoc/current/installing.html#build-environment-on-unix```<br>

(Optional) Adafruit SSD1306 library to address a small (128x64) i2c oled display.
Get it here: ```https://github.com/adafruit/Adafruit_CircuitPython_SSD1306```

If you wish to build and utilize a MongoDB server on your Raspberry Pi 4 (64 bit OS required), I have a written guide here:<br>
https://gist.github.com/EvanGuanSF/9f06f00c1732c582c9714c603fb58f4b

---
### Installation:

- Build the c program and make the data reading script start at boot:
```sudo ./install_bme680_core.sh```<br>

- To create a reporting interval: `sudo ./install_send_bme680_reading.sh -u "userName" -p "password" -l "remoteHost" -d "database" -c "collection" -n "secondsPerDatapoint" -i "[optional] tagString"`
If no `tagString` is provided, the `secondsPerDatapoint` is used instead. Truncates to minute intervals when secondsPerDatapoint > 60. Tags created are of format i.e. `3s` and `15m` for `-n 3` and `-n 900` respectively.

- To remove a specific reporting interval: `sudo ./uninstall_bme680-single.sh tagString`, where tagString is in the format of the tags above.

- To stop and remove all reporting intervals, programs, and services: `sudo ./uninstall_bme680-full.sh`

---
To check active reporting intervals:
For minute+ resolutions: `sudo -u root -l bme680`
For sub-minute intervals: `ps -aux | grep bme680`

---
TODO:
Make install_send_bme680_reading optionally request data from the user to hide credentials from command line history.
