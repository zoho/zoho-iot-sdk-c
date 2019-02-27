CMAKE_MINIMUM_REQUIRED(VERSION 3.10.2)
IF(RUN_TESTS)
    find_library(CMOCKA_LIB NAMES cmocka PATHS /usr/lib)

    add_executable(sdk_basic_tests sdk_basic_tests.c)
    add_test(sdk_basic_tests sdk_basic_tests)
   
    set_property(TARGET sdk_basic_tests PROPERTY LINK_FLAGS "${DEFAULT_LINK_FLAGS} -Wl,--wrap=NetworkConnect,--wrap=MQTTConnect,--wrap=MQTTSubscribe")
    target_link_libraries(sdk_basic_tests ${CMOCKA_LIB} zoho_iot_sdk)
ENDIF(RUN_TESTS)
