#!/bin/bash

############################################################
# Help                                                     #
############################################################
Help()
{
  # Display Help
  echo "Bosch Sensortec BME680 mongo_db logging program configuration and build script."
  echo "Syntax: install_create_bme_reading.sh [-hi] <-upldcn>"
  echo "options:"
  echo "h	help."
  echo "i	i_id_tag (string) For the script being made (Default: time per reading)."
  echo "requirements:"
  echo "u	mongodb_username."
  echo "p	mongodb_password."
  echo "l	mongodb_hostname."
  echo "d	database_name."
  echo "c	collection_name."
  echo "n	n_seconds_per_datapoint. (Truncates to integer minutes if greater than 60)"
  echo
}

############################################################
############################################################
# Main program                                             #
############################################################
############################################################

# Set up variables.
CUR_DIR=$(pwd)
HOME_DIR=$(echo ~)
MONGODB_USERNAME=""
MONGODB_PASSWORD=""
MONGODB_HOSTNAME=""
DATABASE_NAME=""
COLLECTION_NAME=""
N_SECONDS_PER_DATAPOINT=""
# Get whole minute intervals.
I_ID_TAG=""

# Check for help option.
while getopts ":hi:u:p:l:d:c:n:" option; do
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
      N_SECONDS_PER_DATAPOINT="$OPTARG"
      ;;
    i)
      I_ID_TAG="$OPTARG"
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
if [[ $N_SECONDS_PER_DATAPOINT == "" ]]; then
  echo "Error: n_seconds_per_datapoint required."
  PARAMS_OK=1
fi

if [[ $PARAMS_OK == "1" ]]; then
  echo
  echo "Use the -h option for usage."
  exit 1
fi

INSTALL_DIR=/usr/local/sbin/bme680

# Create script id tags if necessary.
N_MINUTES_PER_DATAPOINT=$((N_SECONDS_PER_DATAPOINT / 60))
if (($N_SECONDS_PER_DATAPOINT >= 60)) && test -z $I_ID_TAG
then
I_ID_TAG=""$N_MINUTES_PER_DATAPOINT"m"
fi

if (($N_SECONDS_PER_DATAPOINT < 60)) && test -z $I_ID_TAG
then
I_ID_TAG=""$N_SECONDS_PER_DATAPOINT"s"
fi

# Create a file to hold paths of <60 second scripts.
sudo mkdir -p "$INSTALL_DIR"
sudo touch "$INSTALL_DIR"/bme680_startup.txt

# Create scripts to send data, with operation mode determined by time between inserts.
NEW_SCRIPT_PATH="$INSTALL_DIR"/run_send_bme680_data_"$I_ID_TAG".sh
if (($N_SECONDS_PER_DATAPOINT >= 60))
then
# Create the script.
sudo echo ""$INSTALL_DIR"/send_bme680_data \""$MONGODB_USERNAME"\" \""$MONGODB_PASSWORD"\" \
\""$MONGODB_HOSTNAME"\" \""$DATABASE_NAME"\" \""$COLLECTION_NAME"\" \""$N_SECONDS_PER_DATAPOINT"\"" > "$NEW_SCRIPT_PATH"
sudo chmod +x "$NEW_SCRIPT_PATH"

# Generate a new crontab job to send data to the database.
# Overwrites the old crontab of the same I_ID_TAG if it exists.
CRON_CMD="bash "$NEW_SCRIPT_PATH""
CRON_JOB="*/$N_MINUTES_PER_DATAPOINT * * * * $CRON_CMD"
( crontab -l | grep -v -F "$CRON_CMD" ; echo "$CRON_JOB" ) | crontab -

else
# Remove any old running instances of this script.
pkill -f ".*run_send_bme680_data$I_ID_TAG.sh"

SCRIPTS_FILE="$INSTALL_DIR"/bme680_startup.txt
# If the time interval is 59 seconds or less, we need to run an active looping script as 
# the minimum resolution of cron tab jobs is 1 minute between jobs.
# So, create the script and run it.
sudo echo -e "\
while true\n
do\n
$INSTALL_DIR/send_bme680_data \"$MONGODB_USERNAME\" \"$MONGODB_PASSWORD\" \
\"$MONGODB_HOSTNAME\" \"$DATABASE_NAME\" \"$COLLECTION_NAME\" \"$N_SECONDS_PER_DATAPOINT\"\n
sleep \"$N_SECONDS_PER_DATAPOINT\"\n
done\n" > "$NEW_SCRIPT_PATH"

sudo chmod +x "$NEW_SCRIPT_PATH"
sudo nohup "$NEW_SCRIPT_PATH" >/dev/null 2>&1 &

# Also add the script to the startup scripts file if the same scipt file name
# does not already exist.
sudo grep -qxF -- "$NEW_SCRIPT_PATH" $SCRIPTS_FILE || echo "$NEW_SCRIPT_PATH" >> $SCRIPTS_FILE

fi
