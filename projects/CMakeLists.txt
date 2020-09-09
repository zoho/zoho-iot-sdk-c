IF (BUILD_SAMPLES)
    # Basic example with TLS:
    ADD_SUBDIRECTORY(sampleApplication)

    #include the library in the project root before including projects, if required by other projects else include it in the required projects correspondingly as done for RpiGPIO sample
    INCLUDE(./lib/libmodbus.cmake)
    ADD_SUBDIRECTORY(modbusRequest)
    IF (DEFINED SAMPLES_FOR_PI)
        IF (SAMPLES_FOR_PI)
            ADD_SUBDIRECTORY(rpiGPIO)
        ENDIF()
    ENDIF()
ENDIF()

# Include your projects as sub-directories here
# ADD_SUBDIRECTORY(yourProject)