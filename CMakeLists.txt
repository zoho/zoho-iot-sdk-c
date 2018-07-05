cmake_minimum_required(VERSION 3.10.2)
project(zoho-iot-sdk-c)

set(CMAKE_BUILD_TYPE DEBUG)
include_directories(test include lib/cjson src)
include_directories(lib/paho.mqtt/MQTTPacket/src lib/paho.mqtt/MQTTClient-C/src lib/paho.mqtt/MQTTClient-C/src/linux)

add_subdirectory(lib)


#TODO: remove GLOB
file(GLOB Z_IOT_SOURCES "src/*.c")

add_library(zoho_iot_sdk SHARED ${Z_IOT_SOURCES})
target_link_libraries(zoho_iot_sdk cjson paho-mqtt)

#Tests:
#CJSON:
add_executable(cjson_test test/dependency_tests/cjson_test.c)
target_link_libraries(cjson_test cjson)


#PAHO MQTT:
add_executable(mqtt_test test/dependency_tests/paho/ping.c test/dependency_tests/paho/transport.c)
target_link_libraries(mqtt_test paho-mqtt)