#!/bin/bash
chmod +x zoho_iot_ota/* 
tar -czvf zoho_iot_ota.tar.gz zoho_iot_ota
# Check the exit status of the tar command
if [ $? -ne 0 ]; then
    # Tar command failed
    echo "Error: Failed to create tar archive. Permission denied or other issue."
    exit 1
fi
chmod +x zoho_iot_ota.tar.gz
hash=$(openssl sha256 zoho_iot_ota.tar.gz | awk '{print $2}')
curl_output=$(curl -F "file=@zoho_iot_ota.tar.gz" https://file.io)
link=$(echo "$curl_output" | grep -o '"link":"[^"]*' | cut -d '"' -f 4)
echo "HASH = $hash"
echo "Link = $link"
