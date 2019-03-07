# Zoho IoT C SDK

## Overview

The Zoho IoT SDK for C will let your smart ( gateway ) devices to establish a secure connection with minimal efforts to the Zoho cloud platform. The lightweight MQTT messaging protocol is used to exchange the data between the device & cloud in a secured manner by implementing TLS security.

## Features:

### Connectivity

This SDK will help your device establish a reliable MQTT connection with the server as minimal as much as possible in order to deliver telemetry data from the device and to receive commands from the cloud.

### Security

The communication packets between the device & cloud are secured through TLS implementation.Both Server & Client side X.509 certificate configuration are supported

### Data Collection

This SDK has inbuilt support for JSON data format. It will allow you to effortlessly generate Telemetry payload based on the polled data from sensors. For the advanced usage, it will provide you the flexibility to form your own JSON format.

### Device Portability

This SDK distributed in open sourced form for the effective portability to support variety of devices that runs on different platform and hardware architecture. It has been already built & well tested for the below devices:

- Raspberry Pi

## Build Instructions

### Build Dependencies

Make sure you are running Linux and the following packages are pre installed:

- `CMake` - https://cmake.org
- `unzip`
- `curl`
- `git`

### Getting the Source

> The source of this SDK are located at: `https://git.csez.zohocorpin.com/zoho/zoho-iot-sdk-c`

Run the below command to download the sources:

```
git clone https://git.csez.zohocorpin.com/zoho/zoho-iot-sdk-c
```

### Configure Build parameters

- **TLS support**

To enable TLS support, edit `CMakeList.txt` file and update the value of `Enable TLS for Secure connection` to `ON`.

```
OPTION(ENABLE_TLS "Enable TLS for Secure connection" ON)
```

Configure whether to use Client side certificates or not.

> Default mode will be Server side certificate if you leave this unchanged (OFF)

```
OPTION(USE_CLIENT_CERTS "Use Client side Certs for Secure connection" OFF)
```

Mention the absolute location of your X.509 certificates on the below placeholders

```
SET(CA_CRT "/home/user/mycerts/rootCA.crt")
SET(CLIENT_KEY "/home/user/mycerts/client.key")
SET(CLIENT_CRT "/home/user/mycerts/client.crt")
```

- **Cross Compile Options**

To port this SDK to your custom devices, Refer & clone the cross compile configuration for Raspberry Pi from `cross-compile/raspberry_pi/toolchain.cmake`

And update your custom tool-chain path:

```
#Update your tool-chain path below:
SET (TOOLCHAIN_PATH "/home/user/rpi-toolchain")

#Update your compiler name below:
SET (TOOLCHAIN_COMPILER "arm-bcm2708/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc")
```

Configure your new tool-chain on `/CMakeLists.txt` as shown below

```
SET(CMAKE_TOOLCHAIN_FILE "./cross-compile/raspberry_pi/toolchain.cmake")
```

> The above line will be commented by default. Update and also un-comment it to enable cross compilation.

- **Tying out the basic Example**

This SDK has some default examples to try a basic connection. Please follow the steps to configure it:

Edit the source of `basic.c` and include your deviceID and Token On line # 52

```
rc = zclient_init(&client, "<YOUR-DEVICE-ID>", "<YOUR-DEVICE-TOKEN>", REFERENCE, pRootCACertLocation, pDeviceCertLocation, pDevicePrivateKeyLocation, pDeviceCertParsword);
```

### Build the Source

All the dependent libraries would be automatically downloaded during build .To redownload any dependent library, clear lib folder.

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
> make sure you are in `build` folder and run `./basic_tls` command to tryout the basic example.
