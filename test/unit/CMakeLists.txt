    FIND_LIBRARY(CMOCKA_FOUND NAMES cmocka PATHS /usr/lib)
    FIND_PROGRAM(LCOV_FOUND NAMES lcov)
    FIND_PROGRAM(GCOVR_FOUND NAMES gcovr)
    IF(NOT CMOCKA_FOUND OR NOT GCOVR_FOUND OR NOT LCOV_FOUND)
        MESSAGE(
            FATAL_ERROR
                "Check if dependecies cmocka,lcov & gcovr are pre-installed, if not install them and try again. To run without Testcases please edit the configuration parameters by turning OFF the Z_RUN_TESTS in CMakeLists.txt file and rerun compilation."
            )
    ENDIF()

    add_executable(sdk_basic_tests sdk_basic_tests.c)
    add_test(sdk_basic_tests sdk_basic_tests)
   
    set_property(TARGET sdk_basic_tests PROPERTY LINK_FLAGS "${DEFAULT_LINK_FLAGS} -Wl,--wrap=NetworkConnect,--wrap=MQTTConnect,--wrap=MQTTSubscribe,--wrap=MQTTPublish,--wrap=MQTTDisconnect,--wrap=MQTTYield,--wrap=NetworkConnectTLS,--wrap=NetworkDisconnect")
    target_link_libraries(sdk_basic_tests cmocka zoho_iot_sdk)
