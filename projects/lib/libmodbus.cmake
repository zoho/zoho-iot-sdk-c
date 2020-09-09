include(ExternalProject)
set(MODBUS_LIBRARY_VERSION  libmodbus-3.1.6)
set(MODBUS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/${MODBUS_LIBRARY_VERSION})
set(MODBUS_BIN ${CMAKE_CURRENT_BINARY_DIR}/libmodbus)
set(MODBUS_STATIC_LIB ${MODBUS_BIN}/lib/libmodbus.a)
set(MODBUS_INCLUDES ${MODBUS_BIN}/include)

file(MAKE_DIRECTORY ${MODBUS_INCLUDES})

ExternalProject_Add(
  libmodbus
  PREFIX ${MODBUS_BIN}
  SOURCE_DIR ${MODBUS_DIR}
  URL https://libmodbus.org/releases/${MODBUS_LIBRARY_VERSION}.tar.gz
  CONFIGURE_COMMAND ${MODBUS_DIR}/configure --host=${HOST_FLAG} --srcdir=${MODBUS_DIR} --prefix=${MODBUS_BIN} --enable-static=yes --disable-shared  CC=${CMAKE_C_COMPILER} ${ADDITIONAL_CONFIG_FLAG} --disable-tests  --without-documentation 
  BUILD_COMMAND make
  BUILD_BYPRODUCTS ${MODBUS_STATIC_LIB}
  INSTALL_COMMAND make install
)

add_library(modbus STATIC IMPORTED GLOBAL)

add_dependencies(modbus libmodbus)

set_target_properties(modbus PROPERTIES IMPORTED_LOCATION ${MODBUS_STATIC_LIB})
set_target_properties(modbus PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${MODBUS_INCLUDES})