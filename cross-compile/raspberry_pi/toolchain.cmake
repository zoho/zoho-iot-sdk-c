INCLUDE(CMakeForceCompiler)

SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)

# Update your toolchain root path below:
SET(CMAKE_FIND_ROOT_PATH /mymac/softwares/rpi_toolchain/tools/arm-bcm2708/arm-linux-gnueabihf)
# Update your compiler name below:
SET(TOOLCHAIN_COMPILER /bin/arm-linux-gnueabihf-gcc)

SET(CMAKE_C_COMPILER ${CMAKE_FIND_ROOT_PATH}${TOOLCHAIN_COMPILER})
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
