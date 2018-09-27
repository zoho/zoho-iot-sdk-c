CMAKE_MINIMUM_REQUIRED(VERSION 3.10.2)

#Uncomment the below line and update it with your custom toolchain for cross compilation.
#SET(CMAKE_TOOLCHAIN_FILE "./cross-compile/raspberry_pi/toolchain.cmake")

PROJECT(zoho-iot-sdk-c)
IF (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    MESSAGE( FATAL_ERROR "
                    :::::::::::::::::::::::::

    Only build from LINUX is supported at the moment.
    

                    :::::::::::::::::::::::::" )
ENDIF()

OPTION(ENABLE_TLS "Enable TLS for Secure connection" ON)
OPTION(USE_CLIENT_CERTS "Use Client side Certs for Secure connection" ON)

SET(CA_CRT \"/usr/local/certs/ca.crt\")
SET(CLIENT_CRT \"/usr/local/certs/client.crt\")
SET(CLIENT_KEY \"/usr/local/certs/client.key\")

MESSAGE(STATUS "TLS SUPPORT:" ${ENABLE_TLS})
MESSAGE(STATUS "USE CLIENT CERTS :" ${USE_CLIENT_CERTS})

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

    LIST(APPEND CERTS_PATH  "-DROOTCA_CERT_LOCATION=${CA_CRT}")
    IF(USE_CLIENT_CERTS)
        LIST(APPEND FEATURES "-DUSE_CLIENT_CERTS")
        LIST(APPEND CERTS_PATH  "-DCLIENT_CERT_LOCATION=${CLIENT_CRT}" 
                                "-DCLIENT_KEY_LOCATION=${CLIENT_KEY}")
    ENDIF(USE_CLIENT_CERTS)
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
    TARGET_COMPILE_DEFINITIONS (zoho_iot_sdk PUBLIC ${CERTS_PATH})
ENDIF()

TARGET_LINK_LIBRARIES(zoho_iot_sdk)

##Samples:

#Basic example with TLS:
ADD_EXECUTABLE(basic_tls src/samples/basic.c)
TARGET_LINK_LIBRARIES(basic_tls zoho_iot_sdk)

#Demo on Raspberrypi:
INCLUDE_DIRECTORIES(/usr/local/include)
ADD_EXECUTABLE(demo src/samples/demo.c)
TARGET_LINK_LIBRARIES(demo zoho_iot_sdk )
TARGET_LINK_LIBRARIES(demo "-L/usr/lib/" wiringPi)

##Tests:
#CJSON:
ADD_EXECUTABLE(cjson_test test/dependency_tests/cjson_test.c)
TARGET_LINK_LIBRARIES(cjson_test zoho_iot_sdk)

#PAHO MQTT:
ADD_EXECUTABLE(mqtt_test test/dependency_tests/paho/ping.c test/dependency_tests/paho/transport.c)
TARGET_LINK_LIBRARIES(mqtt_test zoho_iot_sdk)