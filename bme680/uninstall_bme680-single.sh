#!/bin/bash

# Set up variables.
I_ID_TAG=""
I_ID_TAG=$1
INSTALL_DIR=/usr/local/sbin/bme680
SCRIPT_PATH="$INSTALL_DIR"/run_send_bme680_data_"$I_ID_TAG".sh
echo "Stopping and removing job in $SCRIPT_PATH"

# Remove the crontab job.
sudo crontab -l | grep -v "$SCRIPT_PATH" | crontab -
# Remove the line from the startup file.
sudo sed --in-place "/$I_ID_TAG.sh/d" $INSTALL_DIR/bme680_startup.txt

# Stop any old running instance of the data collection scripts/programs.
sudo pkill -f ""$INSTALL_DIR"/run_send_bme680_data_"$I_ID_TAG".sh"

# Clear old files.
sudo rm -rf $SCRIPT_PATH
sudo systemctl daemon-reload
