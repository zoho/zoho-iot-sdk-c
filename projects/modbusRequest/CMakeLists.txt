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

# Uncomment the below if the library is required only for this project and comment the library include in the project root CMakeList.txt file
INCLUDE(../lib/libmodbus.cmake)

ADD_EXECUTABLE(modbusRequest modbusRequest.c)
TARGET_COMPILE_DEFINITIONS(modbusRequest PUBLIC ${FEATURES})
IF(Z_ENABLE_TLS)
    TARGET_COMPILE_DEFINITIONS(modbusRequest PUBLIC ${CERTS_PATH})
ENDIF(Z_ENABLE_TLS)

TARGET_INCLUDE_DIRECTORIES(modbusRequest PUBLIC include )
TARGET_LINK_LIBRARIES(modbusRequest PRIVATE zoho_iot_sdk PRIVATE modbus)