cmake_minimum_required(VERSION 3.10.2)
project(zoho-iot-sdk-c)

set(CMAKE_BUILD_TYPE DEBUG)
include_directories(lib/cjson lib/paho.mqtt/MQTTPacket/src test)

add_subdirectory(lib)

#Tests:
#CJSON:
add_executable(eg test/dependency_tests/cjson_test.c)
target_link_libraries(eg cjson)


#PAHO MQTT:
add_executable(mqtt_test test/dependency_tests/paho/ping.c test/dependency_tests/paho/transport.c)
target_link_libraries(mqtt_test paho-mqtt)