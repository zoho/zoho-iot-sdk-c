# PAHO MQTT LIBRARY :
IF(NOT EXISTS ${PROJECT_SOURCE_DIR}/lib/paho.mqtt)
    SET(PAHO_MQTT_VERSOION v1.0.0)
    MESSAGE("\nDownloading paho.mqtt.embedded-c-${PAHO_MQTT_VERSOION}\n")
    EXECUTE_PROCESS(
        COMMAND curl -LO
                https://github.com/eclipse/paho.mqtt.embedded-c/archive/${PAHO_MQTT_VERSOION}.zip
                # TIMEOUT 900
        RESULT_VARIABLE STATUS)

    IF(NOT ${STATUS} EQUAL 0)
        MESSAGE(
            FATAL_ERROR
                "Can't fetch paho.mqtt.embedded-c library,mention appropriate version or Download manually to lib folder."
            )
    ENDIF()

    EXECUTE_PROCESS(COMMAND unzip -q ${PAHO_MQTT_VERSOION}.zip -d ${PROJECT_SOURCE_DIR}/lib)
    FILE(REMOVE ${PROJECT_BINARY_DIR}/${PAHO_MQTT_VERSOION}.zip)
    EXECUTE_PROCESS(
        COMMAND sed -i s/""/"tls_config.h"/
                ${PROJECT_SOURCE_DIR}/lib/paho.mqtt.embedded-c-1.0.0/MQTTClient-C/src/MQTTClient.h)
    FILE(RENAME ${PROJECT_SOURCE_DIR}/lib/paho.mqtt.embedded-c-1.0.0
                ${PROJECT_SOURCE_DIR}/lib/paho.mqtt)
ENDIF()

# ----------------------------------------------
# CJSON LIBRARY:
IF(NOT EXISTS ${PROJECT_SOURCE_DIR}/lib/cjson)
    SET(CJSON_VERSION v1.7.7)
    MESSAGE("\nDownloading cJSON-${CJSON_VERSION}\n")
    EXECUTE_PROCESS(COMMAND curl -LO
                            https://github.com/DaveGamble/cJSON/archive/${CJSON_VERSION}.zip
                            # TIMEOUT 900
                    RESULT_VARIABLE status)

    IF(NOT ${STATUS} EQUAL 0)
        MESSAGE(
            FATAL_ERROR
                "Can't fetch cjson library,mention appropriate version or Download manually to lib folder."
            )
    ENDIF()

    EXECUTE_PROCESS(COMMAND unzip -q ${CJSON_VERSION}.zip -d ${PROJECT_SOURCE_DIR}/lib)
    FILE(REMOVE ${PROJECT_BINARY_DIR}/${CJSON_VERSION}.zip)
    FILE(RENAME ${PROJECT_SOURCE_DIR}/lib/cJSON-1.7.7 ${PROJECT_SOURCE_DIR}/lib/cjson)
ENDIF()

# --------------------------------------------
# MBED TLS LIBRARY:
IF(NOT EXISTS ${PROJECT_SOURCE_DIR}/lib/mbedtls)
    SET(MBED_TLS_VERSION mbedtls-2.12.0)
    MESSAGE("\nDownloading ${MBED_TLS_VERSION}\n")
    EXECUTE_PROCESS(COMMAND curl -LO
                            https://github.com/ARMmbed/mbedtls/archive/${MBED_TLS_VERSION}.zip
                            # TIMEOUT 900
                    RESULT_VARIABLE status)

    IF(NOT ${STATUS} EQUAL 0)
        MESSAGE(
            FATAL_ERROR
                "Can't fetch mbedtls library,mention appropriate version or Download manually to lib folder."
            )
    ENDIF()

    EXECUTE_PROCESS(COMMAND unzip -q ${MBED_TLS_VERSION}.zip -d ${PROJECT_SOURCE_DIR}/lib)
    FILE(REMOVE ${PROJECT_BINARY_DIR}/${MBED_TLS_VERSION}.zip)
    FILE(RENAME ${PROJECT_SOURCE_DIR}/lib/mbedtls-mbedtls-2.12.0 ${PROJECT_SOURCE_DIR}/lib/mbedtls)
ENDIF()
