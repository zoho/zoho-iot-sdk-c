#!/bin/bash

# Variables are defined in the parent script
APPLICATION_NAME=
APPLICATION_PATH=
SLEEP_TIME=

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

#Function to end the script
end_script() {
    log_message "Zoho ota Service execution ended."
    rm -rf /tmp/zoho_iot_ota
    rm -rf /etc/systemd/system/zoho_iot_ota.service
    exit 0
}

#function to start the service
start_service() {
    systemctl start "$APPLICATION_NAME.service"
    log_message "Started $APPLICATION_NAME.service"
}

# Log script execution start
log_message "zoho ota Service execution started."

#define sleep 2 seconds for zparent.sh to end
sleep 2

# Stop the service
systemctl stop "$APPLICATION_NAME.service"
log_message "Stopped $APPLICATION_NAME.service"

# Sleep for some time to check if the service is stopped
sleep 2
#check if the service is stopped
if [ "$(systemctl is-active "$APPLICATION_NAME.service")" = "active" ]; then
    echo "fail" >> "/zoho_iot_ota_status.txt"
    echo "OTA failed" >> "/zoho_iot_ota_status.txt"
    log_message "OTA failed"
    end_script
fi

# Rename the application required for rollback
mv "$APPLICATION_PATH/$APPLICATION_NAME" "$APPLICATION_PATH/$APPLICATION_NAME.backup"
log_message "Taking backup of $APPLICATION_NAME for rollback"

# Copy the new application to the application directory
cp "/tmp/zoho_iot_ota/$APPLICATION_NAME" "$APPLICATION_PATH/"
log_message "Copying new $APPLICATION_NAME"

# Restart the service
start_service

# Sleep for some time to check if the service is active
sleep "$SLEEP_TIME"

# Check if the service is active
if [ "$(systemctl is-active "$APPLICATION_NAME.service")" = "active" ]; then

    # Write "success" to the file
    echo "success" >> "/tmp/zoho_iot_ota_status.txt"
    echo "OTA successfull" >> "/tmp/zoho_iot_ota_status.txt"
    log_message "OTA successfull"

    # Remove the backup application
    rm "$APPLICATION_PATH/$APPLICATION_NAME.backup"
    log_message "Removing backup $APPLICATION_NAME"
else
    # Write "fail" to the file
    echo "fail" >> "/tmp/zoho_iot_ota_status.txt"
    echo "OTA failed rolling back" >> "/tmp/zoho_iot_ota_status.txt"
    log_message "OTA failed rolling back"
    
    # Roll back the application
    rm "$APPLICATION_PATH/$APPLICATION_NAME"
    mv "$APPLICATION_PATH/$APPLICATION_NAME.backup" "$APPLICATION_PATH/$APPLICATION_NAME"

    # Restart the service
    log_message "Start roll back file"
    start_service
fi


# Log script execution end
end_script