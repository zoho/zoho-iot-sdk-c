#!/bin/bash

# todo: check the existance of curl,tar and unzip commands.

#removing the exising downloaded items on lib folder
rm -rf lib/paho.mqtt
rm -rf lib/cjson

echo "Creating temp  directory"
mkdir tmp && cd tmp

echo "Fetching paho MQTT Library.."
curl -LO https://github.com/eclipse/paho.mqtt.embedded-c/archive/v1.0.0.tar.gz
tar -xvf v1.0.0.tar.gz
mv paho.mqtt.embedded-c-1.0.0 ../lib/paho.mqtt

echo "Fetching cJSON Library.."
curl -LO https://github.com/DaveGamble/cJSON/archive/master.zip
unzip master.zip
mv cJSON-master ../lib/cjson

cd ..
echo "Setup Done!! Cleaning the temp  directory.."
rm -rf tmp
