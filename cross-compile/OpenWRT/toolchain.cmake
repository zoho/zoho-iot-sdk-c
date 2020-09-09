INCLUDE(CMakeForceCompiler)

SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)

# The toolchain rootpart for the Cloudgate device from Option which implements OpenWRT is as below.
# For other OpenWRT based devices, edit the toolchain root path and the compiler correspondingly.

# Update your toolchain root path below:
SET(CMAKE_FIND_ROOT_PATH /Path_to_SDK/cloudgate-sdk-x86_64-{version}/staging_dir/toolchain-arm_v5te_gcc-4.4.3_uClibc-0.9.33.2_eabi/)

# Update your compiler name below:
SET(TOOLCHAIN_COMPILER /bin/arm-openwrt-linux-gcc)
SET(ENV{STAGING_DIR} ${CMAKE_FIND_ROOT_PATH})

SET(CMAKE_C_COMPILER ${CMAKE_FIND_ROOT_PATH}${TOOLCHAIN_COMPILER})

# Additional flags for library compilation:
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE")

# Look for programs in the build host directories.
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Look for libraries and headers in the target directories.
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)