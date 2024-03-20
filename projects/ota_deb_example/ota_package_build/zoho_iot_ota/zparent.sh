#!/bin/bash

# Define variables
# Your application name
PACKAGE_NAME=zoho_iot_ota.deb


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

#Check if the application exists
if [ ! -f "/tmp/zoho_iot_ota/$PACKAGE_NAME" ]; then
    log_message "$PACKAGE_NAME not found"
    echo "/tmp/zoho_iot_ota/$PACKAGE_NAME not found"
    log_message "Parent Script execution ended."
    exit 1
fi

# Command to update the application name
sed_command="sed -i \"s/PACKAGE_NAME=.*/PACKAGE_NAME=$PACKAGE_NAME/g\" /tmp/zoho_iot_ota/zupdate.sh"
# Execute the command 
eval "$sed_command"
# Check the status
if [ ! $? -eq 0 ]; then
    echo "Failed to update package name"
    log_message "Failed to update package name"
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
log_message "Correlation_id: $1"


# Ensure application is executable
chmod +x /tmp/zoho_iot_ota/$PACKAGE_NAME

log_message "Starting zoho_iot_ota.service"
cp /tmp/zoho_iot_ota/zoho_iot_ota.service /etc/systemd/system/
systemctl start zoho_iot_ota.service

# Log message indicating zoho_iot_ota.service has been started
log_message "zoho_iot_ota.service started."

# Log script execution end
log_message "Parent Script execution ended."   