# Zoho IoT C SDK

## Overview

The Zoho IOT SDK for C will let your smart / gateway devices to establish a secure connection with minimal efforts to the Zoho IOT applications. The lightweight MQTT messaging protocol is used to exchange the data between the device & cloud application in a secured manner by implementing TLS security.

## Features:

### Connectivity

This SDK will help your device establish a reliable MQTT connection with the server with minimal overhead in order to deliver telemetry data from the device and to receive commands from the application.

### Security

The communication packets between the device & application are secured through TLS implementation.Both Server & Client side X.509 certificate configuration are supported

### Data Collection

This SDK has inbuilt support for JSON data format. It will allow you to effortlessly generate telemetry payload based on the polled data from sensors. For the advanced requirements, it will provide you with flexibility to form your own JSON format.

### Device Portability

This SDK is distributed in open sourced form for effective portability to support variety of devices that runs on different platforms and have varied hardware architecture. It has been well tested for the below devices:

- Raspberry Pi
- OpenWRT based device 

## Build Instructions

### Build Dependencies

Make sure your device is running Linux and the following packages are pre installed:

- `CMake` - https://cmake.org
- `build-essential`
- `unzip`
- `curl`
- `patch`
- `git`

> Here is the command to install all dependencies: 
> 
>`sudo apt update && sudo apt install cmake build-essential unzip curl patch git`

>NOTE: This SDK is developed and tested on GCC & G++ version 8. If you face any build issues on the different versions, then try again with the specified version.

If you need to run the tests, the following packages need to be installed.

- `cmocka`
- `lcov`
- `covr`

### Getting the Source

> The source of this SDK are located at: `https://github.com/zoho/zoho-iot-sdk-c`

Run the below command to download the sources:

```
git clone https://github.com/zoho/zoho-iot-sdk-c
```

### Configure Build parameters
Edit `CMakeList.txt` file located on root to update the below build configurations:
- **Unit Test Support**

Enable Running test cases by changing Z_RUN_TESTS option from 'OFF' to 'ON'

```
OPTION(Z_RUN_TESTS "Run unit tests" ON)
```

- **TLS support**

To enable TLS support, update the value of `Enable TLS for Secure connection` to `ON`.

```
OPTION(Z_ENABLE_TLS "Enable TLS for Secure connection" ON)
```

Configure whether to use TLS Client certificates or not.

```
OPTION(Z_USE_CLIENT_CERTS "Use Client side Certs for Secure connection" OFF)
```
If TLS mode and client certificate enabled, update your X.509certificate locations as shown below:

```
SET(CA_CRT "/home/user/mycerts/rootCA.crt")
SET(CLIENT_KEY "/home/user/mycerts/client.key")
SET(CLIENT_CRT "/home/user/mycerts/client.crt")
```

### Build the Source

All the dependent libraries will be automatically downloaded during build .To re-download any dependent library, clear lib folder.

Create a build folder to keep all the temporary build configurations

```
mkdir build
cd build
```

And finally invoke the build

```
cmake ..
make
```

> The generated libraries and binary files can be found inside the build folder.

### Tying out the included Examples

This SDK has some implementation examples located on `projects` directory. 

All the examples are commented/excluded out from build process by default in `build/CMakeLists.txt`. Required examples needs to be uncommented before initiating the build.
Follow the instructions to try the basic connectivity example.

Edit `projects/basic/basic.c` and update your ``MqttUserName`` and ``MqttPassword`` On line # 16

```
#define MQTT_USER_NAME "<MQTT User Name>"
#define MQTT_PASSWORD "<MQTT Password/Token>"
```

Edit the logConfig structure in case of changes in logfile name, size and location.

> Once the build process completed, the generated binaries can be found at `build/projects` folder. Swtich to `build/basic` folder and run `./basic` to execute the basic example.

 **Porting/Cross Compile Options :**

Porting or cross-compile instructions are available for below platforms & devices:
- **[Raspberry Pi](cross-compile/raspberry_pi/README.md)**
- **[OpenWRT](cross-compile/OpenWRT/README.md)**

To port this SDK to your custom devices, create a toolchain.cmake file containing the cross-compile configuration for the traget device.

While invoking the build, append -DCMAKE_TOOLCHAIN_FILE= {path}/toolchain.cmake to the cmake command.
```
cmake .. -DCMAKE_TOOLCHAIN_FILE={path}/toolchain.cmake
```
Similar instructions can be followed to support any new custom platforms & devices.
