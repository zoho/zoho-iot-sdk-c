# Porting Zoho IoT C SDK for OpenWRT based Devices

Refer https://openwrt.org/docs/guide-developer/crosscompile  to identify the toolchain, the environmental variables and the compiler required for cross compiling for the OpenWRT based target device.
 
Identify the rootpath of the toolchain which contains compiler,library and include folder necessary for cross-compilation.

In the toolchain.cmake file, set the rootpath as the CMAKE_FIND_ROOT_PATH and point the C compiler with its path relative to the toolchain rootpath. 
Also set the STAGING_DIR environment variable to the toolchain dirictory and export it manually while invoking the build.

Eg:-
```
SET(CMAKE_FIND_ROOT_PATH {toolchain_build_path}/staging_dir/toolchain-architecture_gcc-compilerver_uClibc-libcver/)
SET(TOOLCHAIN_COMPILER /bin/architecture-openwrt-linux-uclibc-gcc)
SET(ENV{STAGING_DIR} ${CMAKE_FIND_ROOT_PATH})
```

```
export STAGING_DIR={toolchain_build_path}/staging_dir/toolchain-architecture_gcc-compilerver_uClibc-libcver/
```
While invoking the build, append -DCMAKE_TOOLCHAIN_FILE=./cross-compile/OpenWRT/toolchain.cmake to the cmake command. 
```
cmake .. -DCMAKE_TOOLCHAIN_FILE=./cross-compile/OpenWRT/toolchain.cmake
```
## Sample toolchain implementation for CloudGate Device:

The current implementation in the toolchain.cmake is provided for the CloudGate device from Option.
The toolchain is obtained from the SDK provided by the OEM. 

```
SET(CMAKE_FIND_ROOT_PATH /Path_to_SDK/cloudgate-sdk-x86_64-{version}/staging_dir/toolchain-arm_v5te_gcc-4.4.3_uClibc-0.9.33.2_eabi/)
SET(TOOLCHAIN_COMPILER /bin/arm-openwrt-linux-gcc)
SET(ENV{STAGING_DIR} ${CMAKE_FIND_ROOT_PATH})
```

##### Additional flags for compiling the library :
Set any target specific flags which are required for the compilation of 3rd part library such as hostname,cflags etc in the toolchain.cmake file itself.

Eg:-
```
SET(HOST_FLAG "arm-openwrt-linux")
SET(ADDITIONAL_CONFIG_FLAG ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes )
```