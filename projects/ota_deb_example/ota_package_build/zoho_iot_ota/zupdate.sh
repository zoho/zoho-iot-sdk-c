#!/bin/bash

# Variables are defined in the parent script
PACKAGE_NAME=

# Define log file path
LOG_FILE="/tmp/zoho_ota_update.log"
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
log_message "zoho ota Service execution started."

# Stop the application
log_message "Stopping zoho_iot_deb.service"
systemctl stop zoho_iot_deb.service

#install application
dpkg -i $PACKAGE_NAME >> "$LOG_FILE" 2>&1

# Check the installation status
if [ $? -eq 0 ]; then
    log_message "OTA successfull"
    echo "success" >> "/tmp/zoho_iot_ota_status.txt"
    echo "OTA successfull" >> "/tmp/zoho_iot_ota_status.txt"
else
    log_message "OTA fail"
    echo "fail" >> "/tmp/zoho_iot_ota_status.txt"
    echo "OTA failed" >> "/tmp/zoho_iot_ota_status.txt"
fi
# Start the application
log_message "Starting zoho_iot_deb.service"
systemctl start zoho_iot_deb.service

# Log script execution end
log_message "Zoho ota Service execution ended."
rm -rf /tmp/zoho_iot_ota
rm -rf /etc/systemd/system/zoho_iot_ota.service
exit 0