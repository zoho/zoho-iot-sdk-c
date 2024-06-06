CMAKE_MINIMUM_REQUIRED(VERSION 3.10.2)
SET(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-fcommon")
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

OPTION(Z_ENABLE_SHARED "Enable SHARED for DYNAMIC lib(.so)" OFF)
OPTION(Z_ENABLE_TLS "Enable TLS for Secure connection" OFF)
OPTION(Z_USE_CLIENT_CERTS "Use Client side Certs for Secure connection" OFF)

#To enable unit test cases:
OPTION(Z_RUN_TESTS "Run unit tests" OFF)

# Set Log level to LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL.
SET(Z_LOG_LEVEL LOG_DEBUG)

# Enable file logging with default SDK configuration, Can be overridden in the apllication using the ZlogConfig struct. 
OPTION(Z_ENABLE_LOGGING "Create logs for Application" OFF)
OPTION(Z_LOG_COMPRESS "Compress the rolling log" ON)
OPTION(Z_STATIC_ZLIB "Use static zlib library" ON)
OPTION(Z_CLOUD_LOGGING "Enable cloud logging" OFF)
OPTION(Z_STATIC_OPENSSL "Use static openssl library" OFF)
OPTION(Z_ENABLE_COVERAGE "Enable code coverage" OFF)
IF(Z_LOG_COMPRESS)
    IF(Z_ENABLE_SHARED AND Z_STATIC_ZLIB)
        MESSAGE(
                FATAL_ERROR
                    "zlib Cannot be static in shared build"
                )
    ENDIF()
ENDIF(Z_LOG_COMPRESS)

# Enable Paho MQTT Client debug logs.
OPTION(Z_PAHO_DEBUG "Enable Paho MQTT Client debug logs" ON)

# Build mode, should be disabled on Production build.
#DEBUG param is required for code coverage report generation.
SET(CMAKE_BUILD_TYPE DEBUG)

# Build Exportable Library sources
OPTION(BUILD_EXPORTABLE_LIB "Build Exportable Library" OFF)
MESSAGE(STATUS "===========================")
MESSAGE(STATUS "ZOHO-IOT-SDK CONFIGURATOINS")
MESSAGE(STATUS "===========================")
MESSAGE(STATUS "TLS SUPPORT : " ${Z_ENABLE_TLS})
MESSAGE(STATUS "USE CLIENT CERTS : " ${Z_USE_CLIENT_CERTS})
MESSAGE(STATUS "LOG LEVEL : " ${Z_LOG_LEVEL})
MESSAGE(STATUS "GENERATE APPLICATION LOGS : " ${Z_ENABLE_LOGGING})
MESSAGE(STATUS "CLOUD LOGS : " ${Z_CLOUD_LOGGING})
MESSAGE(STATUS "Z_STATIC_OPENSSL: " ${Z_STATIC_OPENSSL})
IF(Z_ENABLE_SHARED)
MESSAGE(STATUS "BUILD TYPE: SHARED")
ELSE()
MESSAGE(STATUS "BUILD TYPE: STATIC")
ENDIF(Z_ENABLE_SHARED)

INCLUDE (TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
IF(IS_BIG_ENDIAN)
    MESSAGE(STATUS "BIG_ENDIAN")
    LIST(APPEND FEATURES "-DREVERSED") 
ELSE()
    MESSAGE(STATUS "LITTLE_ENDIAN")
ENDIF(IS_BIG_ENDIAN)


SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

#utility to update depedent libaries.
#ADD_SUBDIRECTORY(cmake/setup)
enable_testing()

IF(Z_ENABLE_COVERAGE)
    include(CodeCoverage)
ENDIF(Z_ENABLE_COVERAGE)

INCLUDE_DIRECTORIES(test include lib/cjson src)
INCLUDE_DIRECTORIES(lib/paho.mqtt/MQTTPacket/src lib/paho.mqtt/MQTTClient-C/src)

FILE(GLOB PAHO_SOURCES "lib/paho.mqtt/MQTTPacket/src/*.c" "lib/paho.mqtt/MQTTClient-C/src/*.c")
LIST(APPEND FEATURES "-DMQTTCLIENT_PLATFORM_HEADER=tls_config.h")
SET(CJSON_SOURCES lib/cjson/cJSON.c)

LIST(APPEND FEATURES "-DZ_LOG_LEVEL=${Z_LOG_LEVEL}")
IF(Z_ENABLE_LOGGING)
    LIST(APPEND FEATURES "-DZ_LOGGING")
ENDIF(Z_ENABLE_LOGGING)

IF(Z_PAHO_DEBUG)
    LIST(APPEND FEATURES "-DZ_PAHO_DEBUG")
ENDIF(Z_PAHO_DEBUG)

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

IF(Z_LOG_COMPRESS)
    LIST(APPEND FEATURES "-DZ_LOG_COMPRESS")
ENDIF(Z_LOG_COMPRESS)
IF(Z_CLOUD_LOGGING)
    LIST(APPEND FEATURES "-DZ_CLOUD_LOGGING")
ENDIF(Z_CLOUD_LOGGING)

IF(Z_ENABLE_SHARED)
ADD_LIBRARY(zoho_iot_sdk SHARED ${SDK_DEPENDENCIES})
ELSE()
ADD_LIBRARY(zoho_iot_sdk STATIC ${SDK_DEPENDENCIES})
ENDIF(Z_ENABLE_SHARED)

TARGET_COMPILE_DEFINITIONS(zoho_iot_sdk PUBLIC ${FEATURES})
set(TARGET_DEPENDENCIES)
IF(Z_CLOUD_LOGGING)
    IF(Z_STATIC_OPENSSL)
        INCLUDE(${PROJECT_SOURCE_DIR}/cmake/libopenssl.cmake)
        add_dependencies(zoho_iot_sdk  openssl_ssl openssl_crypto)
        list(APPEND TARGET_DEPENDENCIES openssl_ssl openssl_crypto)

    ELSE()
        IF(OPENSSL_ROOT_DIR)
            SET(OPENSSL_ROOT_DIR ${OPENSSL_ROOT_DIR})
        ENDIF(OPENSSL_ROOT_DIR)
        FIND_PACKAGE(OpenSSL REQUIRED)
        include_directories(${OPENSSL_INCLUDE_DIR})
        list(APPEND TARGET_DEPENDENCIES OpenSSL::SSL OpenSSL::Crypto)
    ENDIF(Z_STATIC_OPENSSL)

ENDIF(Z_CLOUD_LOGGING)

IF(Z_LOG_COMPRESS)
    IF(Z_STATIC_ZLIB)
    INCLUDE(${PROJECT_SOURCE_DIR}/cmake/zlib.cmake)
        list(APPEND TARGET_DEPENDENCIES zlib)
    ELSE()
        list(APPEND TARGET_DEPENDENCIES z)
    ENDIF(Z_STATIC_ZLIB)
ENDIF(Z_LOG_COMPRESS)

# Link all dependencies to the target at once
TARGET_LINK_LIBRARIES(zoho_iot_sdk PUBLIC ${TARGET_DEPENDENCIES})

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

FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include)
FILE(COPY
${CMAKE_SOURCE_DIR}/include/tls_config.h
${CMAKE_SOURCE_DIR}/include/tls_network.h
${CMAKE_SOURCE_DIR}/include/zclient_constants.h
${CMAKE_SOURCE_DIR}/include/zoho_iot_client.h
${CMAKE_SOURCE_DIR}/include/zoho_log.h
${CMAKE_SOURCE_DIR}/include/zoho_message_handler.h
${CMAKE_SOURCE_DIR}/include/zoho_utils.h
${CMAKE_SOURCE_DIR}/lib/paho.mqtt/MQTTPacket/src/MQTTConnect.h
${CMAKE_SOURCE_DIR}/lib/paho.mqtt/MQTTPacket/src/MQTTFormat.h
${CMAKE_SOURCE_DIR}/lib/paho.mqtt/MQTTPacket/src/MQTTPacket.h
${CMAKE_SOURCE_DIR}/lib/paho.mqtt/MQTTPacket/src/MQTTPublish.h
${CMAKE_SOURCE_DIR}/lib/paho.mqtt/MQTTPacket/src/MQTTSubscribe.h
${CMAKE_SOURCE_DIR}/lib/paho.mqtt/MQTTPacket/src/MQTTUnsubscribe.h
${CMAKE_SOURCE_DIR}/lib/paho.mqtt/MQTTPacket/src/StackTrace.h
${CMAKE_SOURCE_DIR}/lib/paho.mqtt/MQTTClient-C/src/MQTTClient.h
${CMAKE_SOURCE_DIR}/lib/paho.mqtt/MQTTClient-C/src/linux/MQTTLinux.h
${CMAKE_SOURCE_DIR}/lib/cjson/cJSON_Utils.h
${CMAKE_SOURCE_DIR}/lib/cjson/cJSON.h
DESTINATION ${CMAKE_BINARY_DIR}/include
)

 IF(Z_ENABLE_TLS)
 FILE(COPY
 ${CMAKE_SOURCE_DIR}/lib/mbedtls/include/mbedtls
 DESTINATION ${CMAKE_BINARY_DIR}/include
 )
 ENDIF(Z_ENABLE_TLS)

IF(Z_LOG_COMPRESS)
    IF(Z_STATIC_ZLIB)
    ADD_CUSTOM_COMMAND(TARGET libzlib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_BINARY_DIR}/libzlib/include/zlib.h
    ${CMAKE_BINARY_DIR}/libzlib/include/zconf.h
    ${CMAKE_BINARY_DIR}/include
    )
    ENDIF(Z_STATIC_ZLIB)
ENDIF(Z_LOG_COMPRESS)

# Project implementations:
ADD_SUBDIRECTORY(projects)

#Unit Test cases:
IF(Z_RUN_TESTS)
    add_subdirectory(test/unit)
ENDIF(Z_RUN_TESTS)

INSTALL(TARGETS zoho_iot_sdk DESTINATION /usr/local/lib)
INSTALL(DIRECTORY ${CMAKE_BINARY_DIR}/include/ DESTINATION /usr/local/include)
