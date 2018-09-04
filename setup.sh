#!/bin/bash

# todo: check the existance of curl,tar and unzip commands.

#removing the exising downloaded items on lib folder
rm -rf lib/paho.mqtt
rm -rf lib/cjson
rm -rf lib/mbedtls
rm -rf tmp

echo "Creating temp  directory"
mkdir tmp && cd tmp

echo "Fetching paho MQTT Library.."
curl -LO https://github.com/eclipse/paho.mqtt.embedded-c/archive/v1.0.0.zip
unzip v1.0.0.zip

cd paho.mqtt.embedded-c-1.0.0/MQTTClient-C/src/
mv MQTTClient.h MQTTClient.swap
#TODO: platform specific header should be included.
#sed -e 's/""/"MQTTLinux.h"/' MQTTClient.swap > MQTTClient.h
sed -e 's/""/"tls_network.h"/' MQTTClient.swap > MQTTClient.h
rm MQTTClient.swap
cd -
mv paho.mqtt.embedded-c-1.0.0 ../lib/paho.mqtt

echo "Fetching cJSON Library.."
curl -LO https://github.com/DaveGamble/cJSON/archive/v1.7.7.zip
unzip v1.7.7.zip
mv cJSON-1.7.7 ../lib/cjson

echo "Fetching mbedTLS-2.4.1 source ...."
curl -LO https://github.com/ARMmbed/mbedtls/archive/mbedtls-2.12.0.zip
unzip mbedtls-2.12.0.zip
mv mbedtls-mbedtls-2.12.0 ../lib/mbedtls

cd ..
echo "Setup Done!! Cleaning the temp  directory.."
rm -rf tmp
