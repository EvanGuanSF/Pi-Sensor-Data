#!/bin/bash

# Set up variables.
CUR_DIR=$(pwd)
CRON_CMD="$CUR_DIR/run_send_bme680_data.sh"

# Clear old files.
rm -f ~/.pi_sensor_data/bme680_data.txt

# Remove the crontab job.
crontab -l | grep -v "$CRON_CMD"  | crontab -

# Stop any old running instance of the data collection program.
pkill -f ".*get_bme680_data.py"
pkill -f ".*display_bme680_data.py"