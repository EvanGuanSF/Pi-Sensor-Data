#!/bin/bash

# Set up variables.
INSTALL_DIR=/usr/local/sbin/bme680
CRON_CMD="$INSTALL_DIR/run_send_bme680_data.*.sh"

# Stop services.
sudo systemctl stop bme680
sudo systemctl disable bme680
sudo systemctl stop bme680-send
sudo systemctl disable bme680-send

# Remove the crontab job.
sudo crontab -l | grep -v "$CRON_CMD" | crontab -

# Stop any old running instance of the data collection scripts/programs.
sudo pkill -f ".*get_bme680_data.py"
sudo pkill -f ".*run_send_bme680_data.*.sh"

# Clear old files.
sudo rm -rf INSTALL_DIR
sudo systemctl daemon-reload
