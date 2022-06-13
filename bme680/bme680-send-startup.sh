#!/bin/bash

# Starts sub-minute data sending scripts.
SCRIPTS_DIR=/usr/local/sbin/bme680

while IFS= read -r script_path
do
  echo "Starting "$script_path"..."
  nohup "$script_path" >/dev/null 2>&1 &
done < "$SCRIPTS_DIR"/bme680_startup.txt

echo "Startup complete."