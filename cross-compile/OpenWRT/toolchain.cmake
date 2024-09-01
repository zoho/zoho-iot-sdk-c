INCLUDE(CMakeForceCompiler)

SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)

# The toolchain for the Cloudgate device from Option which implements OpenWRT is as below.
# For other OpenWRT based devices, edit the toolchain root path and the compiler correspondingly.

# Update your toolchain root path below:
SET(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_BASE_DIR}/${TOOLCHAIN_ARCH_TYPE})

# Update your compiler name below:
SET(TOOLCHAIN_COMPILER ${TOOLCHAIN_COMPILER})

SET(ENV{STAGING_DIR} ${CMAKE_FIND_ROOT_PATH})


SET(CMAKE_C_COMPILER ${CMAKE_FIND_ROOT_PATH}/${TOOLCHAIN_COMPILER})

# Additional flags for library compilation:
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE")
SET(HOST_FLAG "arm-openwrt-linux")
SET(ADDITIONAL_CONFIG_FLAG ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes )

# Look for programs in the build host directories.
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Look for libraries and headers in the target directories.
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)