IF(RUN_TESTS)

    ENABLE_TESTING()
    IF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

        MESSAGE(STATUS "Settings up flags for Code Coverage")
        SET(GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
        SET(GCC_COVERAGE_LINK_FLAGS "-lgcov")
        # set(GCC_WARNING_COMPILE_FLAGS "-W -Wall")

        SET(CMAKE_C_FLAGS
            "${CMAKE_C_FLAGS} ${GCC_WARNING_COMPILE_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
        SET(CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} ${GCC_WARNING_COMPILE_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

    ENDIF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

ENDIF(RUN_TESTS)