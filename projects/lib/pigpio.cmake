# PIGPIO LIBRARY :
SET(PIGPIO_LIBRARY pigpio-master)
SET(PIGPIO_LIBRARY_ZIP_NAME master)
IF(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PIGPIO_LIBRARY})
    MESSAGE("\n${CMAKE_CURRENT_SOURCE_DIR}/lib/${PIGPIO_LIBRARY}\n")
    MESSAGE("\nDownloading pigpio-master\n")
    EXECUTE_PROCESS(
        COMMAND curl -LO
                https://github.com/joan2937/pigpio/archive/${PIGPIO_LIBRARY_ZIP_NAME}.zip  
                # TIMEOUT 900
        RESULT_VARIABLE STATUS)

    IF(NOT ${STATUS} EQUAL 0)
        MESSAGE(
            FATAL_ERROR
                "Failed to fetch pigpio-master or Download manually to lib folder under the rpi_DHT11 folder."
            )
    ENDIF()

    EXECUTE_PROCESS(COMMAND unzip -q ${PIGPIO_LIBRARY_ZIP_NAME}.zip -d ${CMAKE_CURRENT_SOURCE_DIR}/lib)
    FILE(REMOVE ${PROJECT_BINARY_DIR}/${PIGPIO_LIBRARY_ZIP_NAME}.zip)    
ENDIF()