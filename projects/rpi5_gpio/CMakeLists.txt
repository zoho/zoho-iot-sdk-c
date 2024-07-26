CMAKE_MINIMUM_REQUIRED(VERSION 3.10.2)

SET(CA_CRT /usr/local/certs/ca.crt)
SET(CLIENT_CRT /usr/local/certs/client.crt)
SET(CLIENT_KEY /usr/local/certs/client.key)

# Configure EMBED_CERT_MODE to ON if you want to embed certs along with application. else
# certificates needs to be copied along with application.
OPTION(EMBED_CERT_MODE "Embed certificates along with application" OFF)

INCLUDE(${PROJECT_SOURCE_DIR}/cmake/internalFuncs.cmake)

IF(EMBED_CERT_MODE)
    SET(CRT_PARSE_MODE EMBED)
ELSE()
    SET(CRT_PARSE_MODE REFERENCE)
ENDIF(EMBED_CERT_MODE)

LIST(APPEND FEATURES "-DCRT_PARSE_MODE=${CRT_PARSE_MODE}")
LIST(APPEND FEATURES "-D${CRT_PARSE_MODE}_MODE")
MESSAGE(STATUS "CERTIFICATE CONFIG MODE : " ${CRT_PARSE_MODE})

IF(Z_ENABLE_TLS)
    LIST(APPEND CERTS CA_CRT)
    IF(Z_USE_CLIENT_CERTS)
        LIST(APPEND CERTS CLIENT_CRT CLIENT_KEY)
    ENDIF(Z_USE_CLIENT_CERTS)

    IF(EXISTS ${PROJECT_SOURCE_DIR}/include/zclient_certificates.h)
        FILE(REMOVE ${PROJECT_SOURCE_DIR}/include/zclient_certificates.h)
    ENDIF()

    FOREACH(CERT ${CERTS})
        IF(EMBED_CERT_MODE)
            READ_FILE(${${CERT}} ${CERT})
        ELSE()
            LIST(APPEND CERTS_PATH "-D${CERT}=\"${${CERT}}\"")
        ENDIF(EMBED_CERT_MODE)
    ENDFOREACH(CERT)
ENDIF(Z_ENABLE_TLS)


set(GPIO_LIBRARY_VERSION  libgpiod-1.6.2)
set(GPIOD_LIBRARY_NAME libgpiod-upstream-1.6.2)
set(GPIO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/${GPIOD_LIBRARY_NAME})
set(GPIO_BIN ${CMAKE_CURRENT_BINARY_DIR}/${GPIOD_LIBRARY_NAME})
set(GPIO_STATIC_LIB ${GPIO_BIN}/lib/libgpiod.a)
set(GPIO_INCLUDES ${GPIO_BIN}/include)


# Include the ExternalProject module
include(ExternalProject)

IF(NOT EXISTS ${GPIO_DIR})
    MESSAGE("\n${GPIO_DIR}\n")
    MESSAGE("\nDownloading libgpiod\n")
    EXECUTE_PROCESS(
        COMMAND  curl -L -o ${GPIOD_LIBRARY_NAME}.zip https://github.com/platinasystems/libgpiod/archive/refs/tags/upstream/1.6.2.zip
                # TIMEOUT 900
        RESULT_VARIABLE STATUS)
    
    IF(NOT ${STATUS} EQUAL 0)
        MESSAGE(
            FATAL_ERROR
                "Failed to fetch libgpiod or Download manually to lib forlder under the eip folder."
        )
    ENDIF()
    EXECUTE_PROCESS(COMMAND ls)
    EXECUTE_PROCESS(COMMAND unzip -q ${GPIOD_LIBRARY_NAME}.zip  -d ${CMAKE_CURRENT_SOURCE_DIR}/lib)
    FILE(REMOVE ${PROJECT_BINARY_DIR}/${GPIOD_LIBRARY_NAME}.zip)
ENDIF()

# Define the external project
ExternalProject_Add(libgpiod
    PREFIX ${GPIO_BIN}
    SOURCE_DIR ${GPIO_DIR}
    CONFIGURE_COMMAND ${GPIO_DIR}/configure --prefix=${GPIO_BIN}
    BUILD_COMMAND make  -j1 V=sc
)

# Set the C standard
set(CMAKE_C_STANDARD 99)

# Add the executable
add_executable(rpi5_gpio rpi5_gpio.c)
TARGET_COMPILE_DEFINITIONS(rpi5_gpio PUBLIC ${FEATURES})
IF(Z_ENABLE_TLS)
    TARGET_COMPILE_DEFINITIONS(rpi5_gpio PUBLIC ${CERTS_PATH})
ENDIF(Z_ENABLE_TLS)
# Specify that gpio_interrupt depends on libgpiod
add_dependencies(rpi5_gpio libgpiod)

set_target_properties(rpi5_gpio PROPERTIES IMPORTED_LOCATION ${GPIO_STATIC_LIB})
set_target_properties(rpi5_gpio PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${GPIO_INCLUDES})
target_link_libraries(rpi5_gpio PRIVATE ${GPIO_STATIC_LIB} PRIVATE zoho_iot_sdk PUBLIC pthread)
