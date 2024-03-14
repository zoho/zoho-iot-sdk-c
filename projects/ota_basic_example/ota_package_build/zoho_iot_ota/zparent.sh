#!/bin/bash

# Define variables
# Your application name
APPLICATION_NAME=ota_basic
# Your application directory path
APPLICATION_PATH=/home/admin/ota_basic
# time to sleep after service restart
SLEEP_TIME=20

# Define log file path
LOG_FILE="/tmp/zoho_ota_parent.log"
if [ -f "$LOG_FILE" ]; then
    rm "$LOG_FILE"
fi
touch "$LOG_FILE"

# Function to log messages
log_message() {
    local timestamp=$(date +"%Y-%m-%d %H:%M:%S")
    echo "[$timestamp] $1" >> "$LOG_FILE"
}

# Log script execution start
log_message "Parent Script execution started."

#Check if the service file exists
log_message "Checking service file"
if [ ! -f "/etc/systemd/system/$APPLICATION_NAME.service" ]; then
    log_message "Service file not found"
    echo "$APPLICATION_NAME.service not found"
    log_message "Parent Script execution ended."
    exit 1
fi

#Check if the application exists
if [ ! -f "$APPLICATION_PATH/$APPLICATION_NAME" ]; then
    log_message "$APPLICATION_NAME not found"
    echo "$APPLICATION_PATH/$APPLICATION_NAME not found"
    log_message "Parent Script execution ended."
    exit 1
fi

#Check if the application exists in ota package
if [ ! -f "/tmp/zoho_iot_ota/$APPLICATION_NAME" ]; then
    log_message "/tmp/zoho_iot_ota/$APPLICATION_NAME"
    echo "/tmp/zoho_iot_ota/$APPLICATION_NAME not found"
    log_message "Parent Script execution ended."
    exit 1
fi

#check sleep time is a number
if ! [[ "$SLEEP_TIME" =~ ^[0-9]+$ ]]; then
    log_message "SLEEP_TIME is not a number"
    echo "SLEEP_TIME is not a number"
    log_message "Parent Script execution ended."
    exit 1
fi

# Command to update the application name
sed_command="sed -i \"s/APPLICATION_NAME=.*/APPLICATION_NAME=$APPLICATION_NAME/g\" /tmp/zoho_iot_ota/zupdate.sh"
# Execute the command 
eval "$sed_command"
# Check the status
if [ ! $? -eq 0 ]; then
    echo "Failed to update application name"
    log_message "Failed to update application name"
    log_message "Parent Script execution ended."
    exit 1
fi


#Command to update the application path
sed_command="sed -i \"s@APPLICATION_PATH=.*@APPLICATION_PATH=$APPLICATION_PATH@g\" /tmp/zoho_iot_ota/zupdate.sh"
# Execute the command 
eval "$sed_command"
# Check the status
if [ ! $? -eq 0 ]; then
    echo "Failed to update application path"
    log_message "Failed to update application path"
    log_message "Parent Script execution ended."
    exit 1
fi

# Command to update the sleep time
sed_command="sed -i \"s/SLEEP_TIME=.*/SLEEP_TIME=$SLEEP_TIME/g\" /tmp/zoho_iot_ota/zupdate.sh"
# Execute the command 
eval "$sed_command"
# Check the status
if [ ! $? -eq 0 ]; then
    echo "Failed to update sleep time"
    log_message "Failed to update sleep time"
    log_message "Parent Script execution ended."
    exit 1
fi

# Remove the file if it exists
if [ -f "/tmp/zoho_iot_ota_status.txt" ]; then
    rm "/tmp/zoho_iot_ota_status.txt"
fi

log_message "Creating status file"
# Create the file
touch "/tmp/zoho_iot_ota_status.txt"

log_message "Appending correlation_id to status file"
# Append the correlation_id to the file
echo "$1" >> "/tmp/zoho_iot_ota_status.txt"

# Ensure zupdate.sh is executable
chmod +x /tmp/zoho_iot_ota/zupdate.sh
chmod +x /tmp/zoho_iot_ota/zoho_iot_ota.service

log_message "Starting zoho_iot_ota.service"
cp /tmp/zoho_iot_ota/zoho_iot_ota.service /etc/systemd/system/
systemctl start zoho_iot_ota.service

# Log message indicating zoho_iot_ota.service has been started
log_message "zoho_iot_ota.service started."

# Log script execution end
log_message "Parent Script execution ended."   