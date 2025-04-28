include(ExternalProject)
set(PAHO_LIBRARY_VERSION 1.3.13)  
set(PAHO_DIR ${PROJECT_SOURCE_DIR}/lib/paho.mqtt.c)
set(PAHO_BIN ${CMAKE_CURRENT_BINARY_DIR}/libpaho)
IF(Z_ENABLE_TLS)
    set(PAHO_STATIC_LIB ${PAHO_BIN}/lib/libpaho-mqtt3cs.a)
    set(PAHO_C_TLS TRUE)
ELSE()
    set(PAHO_STATIC_LIB ${PAHO_BIN}/lib/libpaho-mqtt3c.a)
    set(PAHO_C_TLS FALSE)
ENDIF(Z_ENABLE_TLS)
set(PAHO_INCLUDES ${PAHO_BIN}/include)

set(CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DCMAKE_INSTALL_LIBDIR=lib
    -DPAHO_HIGH_PERFORMANCE=TRUE
    -DPAHO_BUILD_STATIC=TRUE
    -DPAHO_WITH_SSL=${PAHO_C_TLS}
    -DPAHO_HIGH_PERFORMANCE=TRUE
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
)

IF(Z_ENABLE_TLS)
    IF(Z_STATIC_OPENSSL)
        set(OPENSSL_BIN ${OPENSSL_BIN})
        set(OPENSSL_INCLUDE_DIR ${OPENSSL_INCLUDES})
        set(OPENSSL_SSL_LIBRARY ${OPENSSL_STATIC_LIB})
        set(OPENSSL_CRYPTO_LIBRARY ${OPENSSL_STATIC_CRYPTO_LIB})
        set(PAHO_STATIC_LIB ${PAHO_BIN}/lib/libpaho-mqtt3cs.a)
        list(APPEND CMAKE_ARGS 
            -DOPENSSL_ROOT_DIR=${OPENSSL_BIN}
            -DOPENSSL_CRYPTO_LIBRARY=${OPENSSL_CRYPTO_LIBRARY}
            -DOPENSSL_INCLUDE_DIR=${OPENSSL_INCLUDE_DIR}
            -DOPENSSL_SSL_LIBRARY=${OPENSSL_SSL_LIBRARY}
        )
    ELSE()
        find_package(OpenSSL REQUIRED)
        message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")
    ENDIF(Z_STATIC_OPENSSL)
ENDIF(Z_ENABLE_TLS)

file(MAKE_DIRECTORY ${PAHO_INCLUDES})


IF(NOT EXISTS ${PAHO_DIR})
    MESSAGE("\nDownloading paho c\n")
    EXECUTE_PROCESS(
        COMMAND wget https://github.com/eclipse/paho.mqtt.c/archive/v${PAHO_LIBRARY_VERSION}.tar.gz
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        TIMEOUT 900
        RESULT_VARIABLE STATUS)
    
    IF(NOT ${STATUS} EQUAL 0)
        MESSAGE(
            FATAL_ERROR
                "Failed to fetch paho c or Download manually to lib folder"
        )
    ENDIF()

    EXECUTE_PROCESS(
        COMMAND tar -xf v${PAHO_LIBRARY_VERSION}.tar.gz -C ${PROJECT_SOURCE_DIR}/lib
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    FILE(REMOVE ${PROJECT_BINARY_DIR}/v-${PAHO_LIBRARY_VERSION}.tar.gz)
    FILE(RENAME ${PROJECT_SOURCE_DIR}/lib/paho.mqtt.c-${PAHO_LIBRARY_VERSION}
                 ${PROJECT_SOURCE_DIR}/lib/paho.mqtt.c)
ENDIF()

ExternalProject_Add(
  libpaho
  PREFIX ${PAHO_BIN}
  SOURCE_DIR ${PAHO_DIR}
  #URL https://github.com/eclipse/paho.mqtt.c/archive/v${PAHO_LIBRARY_VERSION}.tar.gz
  CMAKE_ARGS ${CMAKE_ARGS}
)

add_library(paho STATIC IMPORTED GLOBAL)
add_dependencies(paho libpaho)
IF(Z_STATIC_OPENSSL)
    add_dependencies(libpaho openssl_ssl openssl_crypto)
ENDIF(Z_STATIC_OPENSSL)
set_target_properties(paho PROPERTIES IMPORTED_LOCATION ${PAHO_STATIC_LIB})
set_target_properties(paho PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${PAHO_INCLUDES})

IF(Z_ENABLE_TLS)
    IF(Z_STATIC_OPENSSL)
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${OPENSSL_BIN}/libssl.so.1.1 ${OPENSSL_BIN}/libssl.so
        )
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${OPENSSL_BIN}/libcrypto.so.1.1 ${OPENSSL_BIN}/libcrypto.so
        )

        link_directories(${OPENSSL_BIN})
    ENDIF(Z_STATIC_OPENSSL)
ENDIF(Z_ENABLE_TLS)   
