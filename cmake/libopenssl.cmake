include(ExternalProject)
set(OPENSSL_LIBRARY_VERSION 3.3.1)  # Replace with the desired Paho version
set(OPENSSL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/openssl-${OPENSSL_LIBRARY_VERSION})
set(OPENSSL_BIN ${CMAKE_CURRENT_BINARY_DIR}/libopenssl)
set(OPENSSL_STATIC_LIB ${OPENSSL_BIN}/lib/libssl.a)
set(OPENSSL_STATIC_CRYPTO_LIB ${OPENSSL_BIN}/lib/libcrypto.a)
set(OPENSSL_INCLUDES ${OPENSSL_BIN}/include)

file(MAKE_DIRECTORY ${OPENSSL_INCLUDES})

# Check for lib64
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(OPENSSL_STATIC_LIB ${OPENSSL_BIN}/lib64/libssl.a)
    set(OPENSSL_STATIC_CRYPTO_LIB ${OPENSSL_BIN}/lib64/libcrypto.a)
endif()

#OS_NAME - SHOULD BE DEFINED AT THE TIME OF BUILD. ELSE TAKEN THE SYSTEM OS
MESSAGE("Building static openssl library")
ExternalProject_Add(
  libopenssl
  PREFIX ${OPENSSL_BIN}
  SOURCE_DIR ${OPENSSL_DIR}
  URL https://github.com/openssl/openssl/releases/download/openssl-${OPENSSL_LIBRARY_VERSION}/openssl-${OPENSSL_LIBRARY_VERSION}.tar.gz
  CONFIGURE_COMMAND ${OPENSSL_DIR}/Configure ${OS_NAME} --prefix=${OPENSSL_BIN} -static -fPIC CC=${CMAKE_C_COMPILER} ${ADDITIONAL_CONFIG_FLAG} 
  BUILD_COMMAND make -j4 VERBOSE=1
  BUILD_BYPRODUCTS ${OPENSSL_STATIC_LIB} ${OPENSSL_STATIC_CRYPTO_LIB}
)

add_library(openssl_ssl STATIC IMPORTED GLOBAL)
add_dependencies(openssl_ssl libopenssl)
set_target_properties(openssl_ssl PROPERTIES IMPORTED_LOCATION ${OPENSSL_STATIC_LIB})
set_target_properties(openssl_ssl PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${OPENSSL_INCLUDES})

add_library(openssl_crypto STATIC IMPORTED GLOBAL)
add_dependencies(openssl_crypto libopenssl)
set_target_properties(openssl_crypto PROPERTIES IMPORTED_LOCATION ${OPENSSL_STATIC_CRYPTO_LIB})
set_target_properties(openssl_crypto PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${OPENSSL_INCLUDES})
set_target_properties(openssl_crypto PROPERTIES
    INTERFACE_LINK_LIBRARIES "dl"
)
