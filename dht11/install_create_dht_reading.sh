#!/bin/bash

############################################################
# Help                                                     #
############################################################
Help()
{
  # Display Help
  echo "Digital Temperature and Humidity mongo_db logging program configuration and build script."
  echo "Syntax: install_create_dht_reading.sh [-h] <-upldcn>"
  echo "options:"
  echo "h	help."
  echo "requirements:"
  echo "u	mongodb_username."
  echo "p	mongodb_password."
  echo "l	mongodb_hostname."
  echo "d	database_name."
  echo "c	collection_name."
  echo "n	n_minutes_per_datapoint."
  echo "w	wiring_pi_dht_data_pin."
  echo
}

############################################################
############################################################
# Main program                                             #
############################################################
############################################################

# Set up variables.
CUR_DIR=$(pwd)
MONGODB_USERNAME=""
MONGODB_PASSWORD=""
MONGODB_HOSTNAME=""
DATABASE_NAME=""
COLLECTION_NAME=""
N_MINUTES_PER_DATAPOINT=""
WIRING_PI_DHT_DATA_PIN=""

# Check for help option.
while getopts ":hu:p:l:d:c:n:w:" option; do
  case $option in
    h) # display Help and exit.
      Help
      exit
      ;;
    u)
      MONGODB_USERNAME="$OPTARG"
      ;;
    p)
      MONGODB_PASSWORD="$OPTARG"
      ;;
    l)
      MONGODB_HOSTNAME="$OPTARG"
      ;;
    d)
      DATABASE_NAME="$OPTARG"
      ;;
    c)
      COLLECTION_NAME="$OPTARG"
      ;;
    n)
      N_MINUTES_PER_DATAPOINT="$OPTARG"
      ;;
    w)
      WIRING_PI_DHT_DATA_PIN="$OPTARG"
      ;;
    \?) # Invalid option
      echo "Error: Invalid option"
      exit
      ;;
  esac
done

PARAMS_OK=0

if [[ $MONGODB_USERNAME == "" ]]; then
  echo "Error: mongodb_username required."
  PARAMS_OK=1
fi
if [[ $MONGODB_PASSWORD == "" ]]; then
  echo "Error: mongodb_password required."
  PARAMS_OK=1
fi
if [[ $MONGODB_HOSTNAME == "" ]]; then
  echo "Error: mongodb_hostname required."
  PARAMS_OK=1
fi
if [[ $DATABASE_NAME == "" ]]; then
  echo "Error: database_name required."
  PARAMS_OK=1
fi
if [[ $COLLECTION_NAME == "" ]]; then
  echo "Error: collection_name required."
  PARAMS_OK=1
fi
if [[ $N_MINUTES_PER_DATAPOINT == "" ]]; then
  echo "Error: n_minutes_per_datapoint required."
  PARAMS_OK=1
fi
if [[ $WIRING_PI_DHT_DATA_PIN == "" ]]; then
  echo "Error: wiring_pi_dht_data_pin required."
  PARAMS_OK=1
fi

if [[ $PARAMS_OK == "1" ]]; then
  echo
  echo "Use the -h option for usage."
  exit 1
fi

# Remove the old data file if possible, and replace it with an empty one.
rm -f ~/.pi_sensor_data/dht_data.txt

# Build the programs.
# One to get the data.
gcc -o get_dht_data ./get_dht_data.c -lwiringPi
# And one to send it to the database.
gcc -o send_dht_data ./send_dht_data.c $(pkg-config --libs --cflags libmongoc-1.0)

# Create the run scripts.
# One to get the data.
echo "nohup "$CUR_DIR"/get_dht_data \""$WIRING_PI_DHT_DATA_PIN"\" >/dev/null 2>&1 &" > "$CUR_DIR"/run_get_dht_data.sh
# And one to send it to the database.
echo ""$CUR_DIR"/send_dht_data \""$MONGODB_USERNAME"\" \""$MONGODB_PASSWORD"\" \""$MONGODB_HOSTNAME"\" \
\""$DATABASE_NAME"\" \""$COLLECTION_NAME"\" \""$N_MINUTES_PER_DATAPOINT"\"" > "$CUR_DIR"/run_send_dht_data.sh

chmod +x "$CUR_DIR"/run_get_dht_data.sh
chmod +x "$CUR_DIR"/run_send_dht_data.sh

# Generate a new crontab jobs to send data to the database.
# Overwrite the old one if it exists.
CRON_CMD="$CUR_DIR/run_send_dht_data.sh"
CRON_JOB="*/$N_MINUTES_PER_DATAPOINT * * * * $CRON_CMD"
( crontab -l | grep -v -F "$CRON_CMD" ; echo "$CRON_JOB" ) | crontab -

# Stop any old running instance of the data collection program and start the newly compiled one.
pkill -f ".*get_dht_data"
./run_get_dht_data.sh