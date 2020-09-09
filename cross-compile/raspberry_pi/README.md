# Porting Zoho IoT C SDK for Raspberry Pi

Download the latest toolchain for Raspberry Pi, Ignore if already available.
Identify the rootpath of the toolchain where the compiler,library and includes are available.
In the toolchain.cmake file, set the rootpath as the CMAKE_FIND_ROOT_PATH and point C compiler with its path relative to the toolchain rootpath

Eg:-
```
SET(CMAKE_FIND_ROOT_PATH /path_to_toolchain/tools/arm-bcm2708/arm-linux-gnueabihf)
SET(TOOLCHAIN_COMPILER /bin/arm-linux-gnueabihf-gcc)
```

While invoking the build, append -DCMAKE_TOOLCHAIN_FILE=./cross-compile/raspberry_pi/toolchain.cmake to the cmake command. 
```
cmake .. -DCMAKE_TOOLCHAIN_FILE=./cross-compile/raspberry_pi/toolchain.cmake
```

##### Additional flags for compiling the library :
Set any target specific flags which are required for the compilation of 3rd part library such as hostname,cflags etc in the toolchain.cmake file itself.

Eg:-
```
SET(HOST_FLAG "arm-linux-gnueabihf")
SET(ADDITIONAL_CONFIG_FLAG ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes )
```