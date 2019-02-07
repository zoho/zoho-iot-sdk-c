CMAKE_MINIMUM_REQUIRED(VERSION 3.10.2)
# Uncomment the below line and update it with your custom toolchain for cross compilation.
# SET(CMAKE_TOOLCHAIN_FILE "./cross-compile/raspberry_pi/toolchain.cmake")

PROJECT(zoho-iot-sdk-c)
IF(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    MESSAGE(FATAL_ERROR "
                    :::::::::::::::::::::::::

    Only build from LINUX is supported at the moment.


                    :::::::::::::::::::::::::")
ENDIF()

IF("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    MESSAGE(FATAL_ERROR " In-source builds are not allowed. Please reinitiate from build folder. ")
ENDIF()

# Checking for dependencies, before starting the build.
FIND_PROGRAM(CURL_FOUND NAMES curl)
FIND_PROGRAM(UNZIP_FOUND NAMES unzip)
FIND_PROGRAM(PATCH_FOUND NAMES patch)
IF(NOT UNZIP_FOUND AND NOT CURL_FOUND AND NOT PATCH_FOUND)
    MESSAGE(
        FATAL_ERROR
            "Check if dependecies curl,unzip & patch are pre-installed, if not install them and try again."
        )
ENDIF()

OPTION(ENABLE_TLS "Enable TLS for Secure connection" OFF)
OPTION(USE_CLIENT_CERTS "Use Client side Certs for Secure connection" ON)

#For unit test cases:
OPTION(RUN_TESTS "Run unit tests" ON)

# Set Log level to LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL.
SET(Z_LOG_LEVEL LOG_DEBUG)

# Build mode, should be disabled on Production build.
SET(CMAKE_BUILD_TYPE DEBUG)

MESSAGE(STATUS "TLS SUPPORT : " ${ENABLE_TLS})
MESSAGE(STATUS "USE CLIENT CERTS : " ${USE_CLIENT_CERTS})
MESSAGE(STATUS "LOG LEVEL : " ${Z_LOG_LEVEL})

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
ADD_SUBDIRECTORY(cmake/setup)
include(CodeCoverage)

INCLUDE_DIRECTORIES(test include lib/cjson src)
INCLUDE_DIRECTORIES(lib/paho.mqtt/MQTTPacket/src lib/paho.mqtt/MQTTClient-C/src)

FILE(GLOB PAHO_SOURCES "lib/paho.mqtt/MQTTPacket/src/*.c" "lib/paho.mqtt/MQTTClient-C/src/*.c")
LIST(APPEND FEATURES "-DMQTTCLIENT_PLATFORM_HEADER=tls_config.h")
SET(CJSON_SOURCES lib/cjson/cJSON.c)

LIST(APPEND FEATURES "-DZ_LOG_LEVEL=${Z_LOG_LEVEL}")

IF(ENABLE_TLS)
    INCLUDE_DIRECTORIES(lib/mbedtls/include/)
    FILE(GLOB MBEDTLS_SOURCES "lib/mbedtls/library/*.c")
    LIST(APPEND SDK_DEPENDENCIES ${MBEDTLS_SOURCES})
    LIST(APPEND NETWORK_SOURCES "src/tls_network.c")
    LIST(APPEND FEATURES "-DSECURE_CONNECTION")
    IF(USE_CLIENT_CERTS)
        LIST(APPEND FEATURES "-DUSE_CLIENT_CERTS")
    ENDIF(USE_CLIENT_CERTS)
ELSE()
    INCLUDE_DIRECTORIES(lib/paho.mqtt/MQTTClient-C/src/linux)
    LIST(APPEND NETWORK_SOURCES "lib/paho.mqtt/MQTTClient-C/src/linux/MQTTLinux.c")
ENDIF(ENABLE_TLS)

LIST(APPEND Z_IOT_SOURCES
            "src/iot_client.c"
            "src/log.c"
            "src/utils.c")

LIST(APPEND SDK_DEPENDENCIES
            ${Z_IOT_SOURCES}
            ${CJSON_SOURCES}
            ${PAHO_SOURCES}
            ${NETWORK_SOURCES})

# TODO: Add configurable STATIC/DYNAMIC option.
ADD_LIBRARY(zoho_iot_sdk STATIC ${SDK_DEPENDENCIES})
TARGET_COMPILE_DEFINITIONS(zoho_iot_sdk PUBLIC ${FEATURES})
TARGET_LINK_LIBRARIES(zoho_iot_sdk)

# Samples:

# Basic example with TLS:
ADD_SUBDIRECTORY(samples)

#Unit Test cases:
IF(RUN_TESTS)
    add_subdirectory(test/unit)
ENDIF(RUN_TESTS)

# Tests: CJSON:
ADD_EXECUTABLE(cjson_test test/dependency_tests/cjson_test.c)
TARGET_LINK_LIBRARIES(cjson_test zoho_iot_sdk)

# PAHO MQTT:
ADD_EXECUTABLE(mqtt_test test/dependency_tests/paho/ping.c test/dependency_tests/paho/transport.c)
TARGET_LINK_LIBRARIES(mqtt_test zoho_iot_sdk)
