#Uncomment the example entry `ADD_SUBDIRECTORY(exampleName)` that are required to build.

#1
    #Basic example with TLS:
    #ADD_SUBDIRECTORY(basic)
#2
    #Sample application Modbus protocol support
    #ADD_SUBDIRECTORY(modbusRequest)

#3
    #DHT11 on Raspberry Pi: temperature sensor sample
    #ADD_SUBDIRECTORY(rpi_DHT11)

#4
    #Sample application to do OTA
    #ADD_SUBDIRECTORY(ota_basic_example)
#5
    #Sample application to do OTA for deb pakage
    #ADD_SUBDIRECTORY(ota_deb_example)
    
#6
    #LED on Raspberry Pi: gpio
    #ADD_SUBDIRECTORY(rpi_3_4_gpio)

#7
    #LED on Raspberry Pi 5: gpio
    #ADD_SUBDIRECTORY(rpi5_gpio)



# Include your own projects as sub-directories here
# ADD_SUBDIRECTORY(yourProject)