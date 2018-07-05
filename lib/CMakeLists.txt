cmake_minimum_required(VERSION 3.10.2)

#include the header files directorire of libraries
include_directories(cjson paho.mqtt/MQTTPacket/src)

set(cjson_srcs 
    cjson/cJSON.c
)

file(GLOB SOURCES "paho.mqtt/MQTTPacket/src/*.c")

add_library(cjson SHARED ${cjson_srcs})
add_library(paho-mqtt SHARED ${SOURCES})
