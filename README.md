# Zoho IoT C SDK

Latest Build Status: [![Build Status](http://wn-c7-am-16:8080/buildStatus/icon?job=c-sdk_commits)](http://wn-c7-am-16:8080/job/c-sdk_commits/)

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
- OpenWRT based device (Cloudgate from Option)

## Build Instructions

### Build Dependencies

Make sure you are running Linux and the following packages are pre installed:

- `CMake` - https://cmake.org
- `unzip`
- `curl`
- `git`
If you need to run the test the following packages are required in addition. 
- `cmocka`
- `lcov`
- `covr`

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