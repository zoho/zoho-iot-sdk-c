INCLUDE(CMakeForceCompiler)

SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)

# Update your toolchain root path below:
SET(CMAKE_FIND_ROOT_PATH Path_to_toolchain/tools/arm-bcm2708/arm-linux-gnueabihf)
# Update your compiler name below:
SET(TOOLCHAIN_COMPILER /bin/arm-linux-gnueabihf-gcc)

SET(CMAKE_C_COMPILER ${CMAKE_FIND_ROOT_PATH}${TOOLCHAIN_COMPILER})

# Additional flags for 3rd party library compilation:
SET(HOST_FLAG "arm-linux-gnueabihf")
SET(ADDITIONAL_CONFIG_FLAG ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes )

# Build Sample specific for Raspberry Pi
OPTION(SAMPLES_FOR_PI "Build Samples for RPi" ON)

# Look for programs in the build host directories.
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Look for libraries and headers in the target directories.
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)