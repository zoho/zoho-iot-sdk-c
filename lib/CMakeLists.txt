cmake_minimum_required(VERSION 3.10.2)

#include the header files directorire of libraries
include_directories(cjson paho.mqtt/MQTTPacket/src paho.mqtt/MQTTClient-C/src paho.mqtt/MQTTClient-C/src/linux )

set(cjson_srcs 
    cjson/cJSON.c
)

#TODO: remove GLOB
file(GLOB SOURCES "paho.mqtt/MQTTPacket/src/*.c"  "paho.mqtt/MQTTClient-C/src/linux/*.c" "paho.mqtt/MQTTClient-C/src/*.c" )

add_library(cjson SHARED ${cjson_srcs})
add_library(paho-mqtt SHARED ${SOURCES})
