include(ExternalProject)
set(LIBRARY_VERSION "1.3")
set(ZLIB_DIR ${PROJECT_SOURCE_DIR}/lib/zlib)
set(ZLIB_BIN ${CMAKE_CURRENT_BINARY_DIR}/libzlib)
set(ZLIB_STATIC_LIB ${ZLIB_BIN}/lib/libz.a)
set(ZLIB_INCLUDES ${ZLIB_BIN}/include)
file(MAKE_DIRECTORY ${ZLIB_INCLUDES})

IF(NOT EXISTS ${ZLIB_DIR})
    MESSAGE("\nDownloading zlib\n")
    EXECUTE_PROCESS(
        COMMAND curl -LO https://github.com/madler/zlib/releases/download/v${LIBRARY_VERSION}/zlib-${LIBRARY_VERSION}.tar.gz
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        TIMEOUT 900
        RESULT_VARIABLE STATUS)
    
    IF(NOT ${STATUS} EQUAL 0)
        MESSAGE(
            FATAL_ERROR
                "Failed to fetch zlib or Download manually to lib folder"
        )
    ENDIF()

    EXECUTE_PROCESS(
        COMMAND tar -xf zlib-${LIBRARY_VERSION}.tar.gz -C ${PROJECT_SOURCE_DIR}/lib
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    FILE(REMOVE ${PROJECT_BINARY_DIR}/zlib-${LIBRARY_VERSION}.tar.gz)
    FILE(RENAME ${PROJECT_SOURCE_DIR}/lib/zlib-${LIBRARY_VERSION}
                 ${PROJECT_SOURCE_DIR}/lib/zlib)
ENDIF()

ExternalProject_Add(
  libzlib
  PREFIX ${ZLIB_BIN}
  SOURCE_DIR ${ZLIB_DIR}
#  URL https://zlib.net/zlib-${LIBRARY_VERSION}.tar.gz
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
)

add_library(zlib STATIC IMPORTED GLOBAL)
add_dependencies(zlib libzlib)
set_target_properties(zlib PROPERTIES IMPORTED_LOCATION ${ZLIB_STATIC_LIB})
set_target_properties(zlib PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${ZLIB_INCLUDES})
