#!/bin/bash

############################################################
# Help                                                     #
############################################################
Help()
{
  # Display Help
  echo "Bosch Sensortec BME680 sensor reader and reporter."
  echo
  echo "Sets up scripts and programs for accessing, reading, and"
  echo "sending sensor data to remote MongoDB servers."
  echo
  echo "A Python script reads and shares data via posix shared memory mapping;"
  echo "It uses systemd to launch at startup."
  echo
  echo "A C program is compiled and called on-demand to send data to MongoDB servers."
  echo
  echo "Usage: sudo ./install_bme680_core.sh"
}

############################################################
# Main program                                             #
############################################################

# Check for help option.
while getopts ":h" option; do
  case $option in
    h) # display Help and exit.
      Help
      exit
      ;;
    \?) # Invalid option
      echo "Error: Invalid option"
      exit
      ;;
  esac
done

# Set up variables.
CUR_DIR=$(pwd)
INSTALL_DIR=/usr/local/sbin/bme680

# Create a directory to hold scripts and programs.
sudo mkdir -p "$INSTALL_DIR"
sudo chmod 744 "$INSTALL_DIR"

############################################################
# Python sensor reading script                             #
############################################################
# Copy the python script to the sbin folder.
sudo cp -f "$CUR_DIR"/get_bme680_data.py /usr/local/sbin/bme680/

# Copy the service that runs the data reading python script to
# the main systemd directory.
sudo rm /etc/systemd/system/bme680.service
sudo cp -f "$CUR_DIR"/bme680.service /etc/systemd/system

# Stop any old running instance of the data collection service and restart it.
sudo systemctl daemon-reload
sudo systemctl enable bme680
sudo systemctl restart bme680

############################################################
# Sub-minute data send scripts (Run on startup)            #
############################################################
# Copy the script to be run.
sudo cp -f "$CUR_DIR"/bme680-send-startup.sh /usr/local/sbin/bme680/
sudo chmod 744 "$CUR_DIR"/bme680-send-startup.sh /usr/local/sbin/bme680/

# Copy the service.
sudo rm /etc/systemd/system/bme680-send.service
sudo cp -f "$CUR_DIR"/bme680-send.service /etc/systemd/system

# Stop any old running instance of the service and restart it.
sudo systemctl daemon-reload
sudo systemctl enable bme680-send

############################################################
# C data sending program                                   #
############################################################
# Create a file to hold paths of <60 second scripts.
sudo touch "$INSTALL_DIR"/bme680_startup.txt

# Build the program that sends data to the database.
sudo gcc -o "$INSTALL_DIR"/send_bme680_data ./send_bme680_data.c -lrt -lpthread $(pkg-config --libs --cflags libmongoc-1.0)
sudo chmod +x "$INSTALL_DIR"/send_bme680_data

echo "Installation complete. All created files in: "$INSTALL_DIR""