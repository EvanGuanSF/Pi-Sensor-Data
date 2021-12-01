This program reads sensor data from a DHT11 module on a Raspberry Pi and sends the resulting data to a user designated MongoDB server.

### Requirements:<br>
```sudo apt-get install -y wiringpi crontab gcc pkg-config```
- wiringpi for gpio usage
- crontab for timed jobs
- gcc to build the program
- pkg-config for generating gcc parameters

We need the mongoc driver to access MongoDB databases. Build and install it as per:<br>
```http://mongoc.org/libmongoc/current/installing.html#build-environment-on-unix```<br>

If you wish to build and utilize a MongoDB server on your Raspberry Pi 4 (64 bit OS required), I have a written guide here:<br>
https://gist.github.com/JammyPajamies/9f06f00c1732c582c9714c603fb58f4b

---
### Installation:
Build the program, configure the run scripts, and add a crontab job for run script execution:<br>
```./install.sh -u mongodb_username -p mongodb-password -h mongodb_hostname -d database_name -c collection_name -n n_minutes_per_datapoint -w wiring_pi_data_pin```<br>
The program should now get sensor data from the pin every n_minutes_per_datapoint minutes and write that data to your mongodb server.
---
To check if the cronjob is active, run:
```sudo crontab -l```