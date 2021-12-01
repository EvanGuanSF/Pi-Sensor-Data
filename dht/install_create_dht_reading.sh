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

# ./install_create_dht_reading.sh -u admin -p passwd -l pidb.hopto.me -d pi_sensor_data -c test1 -n 5

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

# Create the run script.
echo ""$CUR_DIR"/create_dht_reading \""$MONGODB_USERNAME"\" \""$MONGODB_PASSWORD"\" \""$MONGODB_HOSTNAME"\" \
\""$DATABASE_NAME"\" \""$COLLECTION_NAME"\" \""$N_MINUTES_PER_DATAPOINT"\" \""$WIRING_PI_DHT_DATA_PIN"\"" > "$CUR_DIR"/run_create_dht_reading.sh

chmod +x "$CUR_DIR"/run_create_dht_reading.sh

# Get the current directory.
CUR_PATH=$(pwd)

# Build the program.
gcc -o create_dht_reading ./create_dht_reading.c ./send_sensor_data.h ./send_sensor_data.c ./get_dht_data.h ./get_dht_data.c $(pkg-config --libs --cflags libmongoc-1.0) -lwiringPi

# Generate a new crontab job.
CRON_CMD="$CUR_DIR/run_create_dht_reading.sh"
CRON_JOB="*/$N_MINUTES_PER_DATAPOINT * * * * $CRON_CMD"
( crontab -l | grep -v -F "$CRON_CMD" ; echo "$CRON_JOB" ) | crontab -