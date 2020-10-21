CMAKE_MINIMUM_REQUIRED(VERSION 3.10.2)
SET(CMAKE_C_STANDARD 99)

PROJECT(zoho-iot-sdk-c)

# For cross compilation call cmake appending -DCMAKE_TOOLCHAIN_FILE= toolchain.cmake for the corresponding target device.
# Example implementation of toolchain.cmake file for some devices is available under cross-compile folder. 

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
IF(NOT UNZIP_FOUND OR NOT CURL_FOUND OR NOT PATCH_FOUND)
    MESSAGE(
        FATAL_ERROR
            "Check if dependecies curl,unzip & patch are pre-installed, if not install them and try again."
        )
ENDIF()

OPTION(Z_ENABLE_TLS "Enable TLS for Secure connection" OFF)
OPTION(Z_USE_CLIENT_CERTS "Use Client side Certs for Secure connection" OFF)

#For unit test cases:
OPTION(RUN_TESTS "Run unit tests" ON)

# Set Log level to LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL.
SET(Z_LOG_LEVEL LOG_DEBUG)

# Build mode, should be disabled on Production build.
SET(CMAKE_BUILD_TYPE DEBUG)

# Build samples
OPTION(BUILD_SAMPLES "Build Samples" ON)

# Build Exportable Library sources
OPTION(BUILD_EXPORTABLE_LIB "Build Exportable Library" OFF)

MESSAGE(STATUS "TLS SUPPORT : " ${Z_ENABLE_TLS})
MESSAGE(STATUS "USE CLIENT CERTS : " ${Z_USE_CLIENT_CERTS})
MESSAGE(STATUS "LOG LEVEL : " ${Z_LOG_LEVEL})
MESSAGE(STATUS "Build Samples : " ${BUILD_SAMPLES})

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
ADD_SUBDIRECTORY(cmake/setup)
include(CodeCoverage)

INCLUDE_DIRECTORIES(test include lib/cjson src)
INCLUDE_DIRECTORIES(lib/paho.mqtt/MQTTPacket/src lib/paho.mqtt/MQTTClient-C/src)

FILE(GLOB PAHO_SOURCES "lib/paho.mqtt/MQTTPacket/src/*.c" "lib/paho.mqtt/MQTTClient-C/src/*.c")
LIST(APPEND FEATURES "-DMQTTCLIENT_PLATFORM_HEADER=tls_config.h")
SET(CJSON_SOURCES lib/cjson/cJSON.c)

LIST(APPEND FEATURES "-DZ_LOG_LEVEL=${Z_LOG_LEVEL}")

IF(Z_ENABLE_TLS)
    INCLUDE_DIRECTORIES(lib/mbedtls/include/)
    FILE(GLOB MBEDTLS_SOURCES "lib/mbedtls/library/*.c")
    LIST(APPEND SDK_DEPENDENCIES ${MBEDTLS_SOURCES})
    LIST(APPEND NETWORK_SOURCES "src/tls_network.c")
    LIST(APPEND FEATURES "-DZ_SECURE_CONNECTION")
    IF(Z_USE_CLIENT_CERTS)
        LIST(APPEND FEATURES "-DZ_USE_CLIENT_CERTS")
    ENDIF(Z_USE_CLIENT_CERTS)
ELSE()
    INCLUDE_DIRECTORIES(lib/paho.mqtt/MQTTClient-C/src/linux)
    LIST(APPEND NETWORK_SOURCES "lib/paho.mqtt/MQTTClient-C/src/linux/MQTTLinux.c")
ENDIF(Z_ENABLE_TLS)

LIST(APPEND Z_IOT_SOURCES
            "src/zoho_iot_client.c"
            "src/zoho_message_handler.c"
            "src/zoho_log.c"
            "src/zoho_utils.c")

LIST(APPEND SDK_DEPENDENCIES
            ${PAHO_SOURCES}
            ${NETWORK_SOURCES}
            ${CJSON_SOURCES}
            ${Z_IOT_SOURCES})

# TODO: Add configurable STATIC/DYNAMIC option.
ADD_LIBRARY(zoho_iot_sdk STATIC ${SDK_DEPENDENCIES})
TARGET_COMPILE_DEFINITIONS(zoho_iot_sdk PUBLIC ${FEATURES})
TARGET_LINK_LIBRARIES(zoho_iot_sdk)

IF (BUILD_EXPORTABLE_LIB)
    add_custom_command(TARGET zoho_iot_sdk PRE_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/ $<TARGET_FILE_DIR:zoho_iot_sdk>/zoho_IoT_Library/src/
                       COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/include/ $<TARGET_FILE_DIR:zoho_iot_sdk>/zoho_IoT_Library/include/
                       COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/lib/paho.mqtt/MQTTPacket/src/*.c $<TARGET_FILE_DIR:zoho_iot_sdk>/zoho_IoT_Library/src/          
                       COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/lib/paho.mqtt/MQTTClient-C/src/*.c $<TARGET_FILE_DIR:zoho_iot_sdk>/zoho_IoT_Library/src/          
                       COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/lib/paho.mqtt/MQTTPacket/src/*.h $<TARGET_FILE_DIR:zoho_iot_sdk>/zoho_IoT_Library/include/
                       COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/lib/paho.mqtt/MQTTClient-C/src/*.h $<TARGET_FILE_DIR:zoho_iot_sdk>/zoho_IoT_Library/include/
                       COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/lib/cjson/cJSON.c $<TARGET_FILE_DIR:zoho_iot_sdk>/zoho_IoT_Library/src/          
                       COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/lib/cjson/*.h $<TARGET_FILE_DIR:zoho_iot_sdk>/zoho_IoT_Library/include/
                      )
    IF(Z_ENABLE_TLS)
         add_custom_command(TARGET zoho_iot_sdk PRE_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/lib/mbedtls/library/*.c $<TARGET_FILE_DIR:zoho_iot_sdk>/zoho_IoT_Library/src/         
                            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:zoho_iot_sdk>/zoho_IoT_Library/include/mbedtls/
                            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/lib/mbedtls/include/mbedtls/*.h $<TARGET_FILE_DIR:zoho_iot_sdk>/zoho_IoT_Library/include/mbedtls/ 
                           )
    ELSE()
         add_custom_command(TARGET zoho_iot_sdk PRE_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/lib/paho.mqtt/MQTTClient-C/src/linux/MQTTLinux.c $<TARGET_FILE_DIR:zoho_iot_sdk>/zoho_IoT_Library/src/           
                            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/lib/paho.mqtt/MQTTClient-C/src/linux/*.h $<TARGET_FILE_DIR:zoho_iot_sdk>/zoho_IoT_Library/include/ 
                           )                               
    ENDIF(Z_ENABLE_TLS)
ENDIF(BUILD_EXPORTABLE_LIB)

# Project implementations:
ADD_SUBDIRECTORY(projects)

#Unit Test cases:
IF(RUN_TESTS)
    add_subdirectory(test/unit)
ENDIF(RUN_TESTS)