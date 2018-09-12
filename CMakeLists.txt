CMAKE_MINIMUM_REQUIRED(VERSION 3.10.2)
PROJECT(zoho-iot-sdk-c)



IF (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    MESSAGE( FATAL_ERROR "
                    :::::::::::::::::::::::::

    Only build from LINUX is supported at the moment.
    

                    :::::::::::::::::::::::::" )
ENDIF()

OPTION(ENABLE_TLS "Enable TLS for Secure connection" ON)
MESSAGE(STATUS "TLS SUPPORT:" ${ENABLE_TLS})

#TODO: avoid GLOB

SET(CMAKE_BUILD_TYPE DEBUG)
INCLUDE_DIRECTORIES(test include lib/cjson src)
INCLUDE_DIRECTORIES(lib/paho.mqtt/MQTTPacket/src lib/paho.mqtt/MQTTClient-C/src)

FILE(GLOB PAHO_SOURCES "lib/paho.mqtt/MQTTPacket/src/*.c"  "lib/paho.mqtt/MQTTClient-C/src/*.c")
SET(CJSON_SOURCES lib/cjson/cJSON.c)

IF(ENABLE_TLS)
    INCLUDE_DIRECTORIES(lib/mbedtls/include/)
    FILE(GLOB MBEDTLS_SOURCES "lib/mbedtls/library/*.c")
    LIST(APPEND SDK_DEPENDENCIES ${MBEDTLS_SOURCES})
    LIST(APPEND NETWORK_SOURCES "src/tls_network.c")
    LIST(APPEND FEATURES "-DSECURE_CONNECTION")
    SET(CERTS_LOCATION "/mymac/Documents/simple_tls/certs/trail/")
ELSE()
    INCLUDE_DIRECTORIES(lib/paho.mqtt/MQTTClient-C/src/linux)
    LIST(APPEND NETWORK_SOURCES "lib/paho.mqtt/MQTTClient-C/src/linux/MQTTLinux.c" )
ENDIF(ENABLE_TLS)

LIST(APPEND Z_IOT_SOURCES "src/iot_client.c" "src/log.c" "src/utils.c")


LIST(APPEND SDK_DEPENDENCIES ${Z_IOT_SOURCES} ${CJSON_SOURCES} ${PAHO_SOURCES} ${NETWORK_SOURCES} )

#TODO: Add configurable STATIC/DYNAMIC option.
ADD_LIBRARY(zoho_iot_sdk STATIC ${SDK_DEPENDENCIES})

IF(ENABLE_TLS)
    TARGET_COMPILE_DEFINITIONS(zoho_iot_sdk PUBLIC ${FEATURES})
ENDIF()

TARGET_LINK_LIBRARIES(zoho_iot_sdk)

##Samples:

#Basic example with TLS:
ADD_EXECUTABLE(basic src/samples/basic.c)
TARGET_LINK_LIBRARIES(basic zoho_iot_sdk)

##Tests:
#CJSON:
ADD_EXECUTABLE(cjson_test test/dependency_tests/cjson_test.c)
TARGET_LINK_LIBRARIES(cjson_test zoho_iot_sdk)

#PAHO MQTT:
ADD_EXECUTABLE(mqtt_test test/dependency_tests/paho/ping.c test/dependency_tests/paho/transport.c)
TARGET_LINK_LIBRARIES(mqtt_test zoho_iot_sdk)