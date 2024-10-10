include(ExternalProject)
set(PAHO_LIBRARY_VERSION 1.3.13)  # Replace with the desired Paho version
set(PAHO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/paho.mqtt.c)
set(PAHO_BIN ${CMAKE_CURRENT_BINARY_DIR}/libpaho)
IF(Z_ENABLE_TLS)
set(PAHO_STATIC_LIB ${PAHO_BIN}/lib/libpaho-mqtt3cs.a)
set(PAHO_C_TLS TRUE)
ELSE()
set(PAHO_STATIC_LIB ${PAHO_BIN}/lib/libpaho-mqtt3c.a)
set(PAHO_C_TLS FALSE)
ENDIF(Z_ENABLE_TLS)
set(PAHO_INCLUDES ${PAHO_BIN}/include)

file(MAKE_DIRECTORY ${PAHO_INCLUDES})
message(STATUS "OPENSSL_BIN: ${OPENSSL_BIN}")
ExternalProject_Add(
  libpaho
  PREFIX ${PAHO_BIN}
  SOURCE_DIR ${PAHO_DIR}
  URL https://github.com/eclipse/paho.mqtt.c/archive/v${PAHO_LIBRARY_VERSION}.tar.gz
  CMAKE_ARGS 
          -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> 
          -DCMAKE_INSTALL_LIBDIR=lib 
          -DPAHO_HIGH_PERFORMANCE=TRUE 
          -DPAHO_BUILD_STATIC=TRUE 
          -DPAHO_WITH_SSL=${PAHO_C_TLS} 
          -DPAHO_HIGH_PERFORMANCE=TRUE 
          -DOPENSSL_ROOT_DIR=${OPENSSL_BIN}
          -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
)

add_library(paho STATIC IMPORTED GLOBAL)
add_dependencies(paho libpaho)
set_target_properties(paho PROPERTIES IMPORTED_LOCATION ${PAHO_STATIC_LIB})
set_target_properties(paho PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${PAHO_INCLUDES})
