# PAHO MQTT LIBRARY :
IF(NOT EXISTS ${PROJECT_SOURCE_DIR}/lib/paho.mqtt)
    SET(PAHO_MQTT_VERSOION v1.1.0)
    MESSAGE("\nDownloading paho.mqtt.embedded-c-${PAHO_MQTT_VERSOION}\n")
    EXECUTE_PROCESS(
        COMMAND curl -LO
                https://github.com/eclipse/paho.mqtt.embedded-c/archive/${PAHO_MQTT_VERSOION}.zip
                # TIMEOUT 900
        RESULT_VARIABLE STATUS)

    IF(NOT ${STATUS} EQUAL 0)
        MESSAGE(
            FATAL_ERROR
                "Failed to fetch paho.mqtt.embedded-c library,mention appropriate version or Download manually to lib folder."
            )
    ENDIF()

    EXECUTE_PROCESS(COMMAND unzip -q ${PAHO_MQTT_VERSOION}.zip -d ${PROJECT_SOURCE_DIR}/lib)
    FILE(REMOVE ${PROJECT_BINARY_DIR}/${PAHO_MQTT_VERSOION}.zip)
    FILE(RENAME ${PROJECT_SOURCE_DIR}/lib/paho.mqtt.embedded-c-1.1.0
                ${PROJECT_SOURCE_DIR}/lib/paho.mqtt)
    EXECUTE_PROCESS(COMMAND patch -p1 -i ${PROJECT_SOURCE_DIR}/cmake/patches/paho_1.3.patch -d
                            ${PROJECT_SOURCE_DIR}/lib/paho.mqtt
                    RESULT_VARIABLE PATCH_STATUS)
    IF(NOT ${PATCH_STATUS} EQUAL 0)
        MESSAGE(FATAL_ERROR "Failed to apply patch on paho.mqtt.embedded-c library.")
        FILE(REMOVE_RECURSE ${PROJECT_SOURCE_DIR}/lib/paho.mqtt)
    ENDIF()
ENDIF()

# ----------------------------------------------
# CJSON LIBRARY:
IF(NOT EXISTS ${PROJECT_SOURCE_DIR}/lib/cjson)
    SET(CJSON_VERSION 1.7.15)
    MESSAGE("\nDownloading cJSON-${CJSON_VERSION}\n")
    EXECUTE_PROCESS(COMMAND curl -LO
                            https://github.com/DaveGamble/cJSON/archive/v${CJSON_VERSION}.zip
                            # TIMEOUT 900
                    RESULT_VARIABLE status)

    IF(NOT ${STATUS} EQUAL 0)
        MESSAGE(
            FATAL_ERROR
                "Failed to fetch cjson library,mention appropriate version or Download manually to lib folder."
            )
            
    ENDIF()

    EXECUTE_PROCESS(COMMAND unzip -q v${CJSON_VERSION}.zip -d ${PROJECT_SOURCE_DIR}/lib)
    FILE(REMOVE ${PROJECT_BINARY_DIR}/v${CJSON_VERSION}.zip)
    FILE(RENAME ${PROJECT_SOURCE_DIR}/lib/cJSON-${CJSON_VERSION} ${PROJECT_SOURCE_DIR}/lib/cjson)
ENDIF()

# --------------------------------------------
# MBED TLS LIBRARY:
IF(NOT EXISTS ${PROJECT_SOURCE_DIR}/lib/mbedtls)
    SET(MBED_TLS_VERSION mbedtls-2.16.3)
    MESSAGE("\nDownloading ${MBED_TLS_VERSION}\n")
    EXECUTE_PROCESS(COMMAND curl -LO
                            https://github.com/ARMmbed/mbedtls/archive/${MBED_TLS_VERSION}.zip
                            # TIMEOUT 900
                    RESULT_VARIABLE status)

    IF(NOT ${STATUS} EQUAL 0)
        MESSAGE(FATAL_ERROR
                "Failed to fetch mbedtls library,mention appropriate version or Download manually to lib folder."
            )
    ENDIF()

    EXECUTE_PROCESS(COMMAND unzip -q ${MBED_TLS_VERSION}.zip -d ${PROJECT_SOURCE_DIR}/lib )
    FILE(REMOVE ${PROJECT_BINARY_DIR}/${MBED_TLS_VERSION}.zip)
    FILE(RENAME ${PROJECT_SOURCE_DIR}/lib/mbedtls-${MBED_TLS_VERSION} ${PROJECT_SOURCE_DIR}/lib/mbedtls)
ENDIF()
