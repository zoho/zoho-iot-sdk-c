cmake_minimum_required(VERSION 3.10.2)
project(zoho-iot-sdk-c)

set(CMAKE_BUILD_TYPE DEBUG)
include_directories(lib/cjson)

add_subdirectory(lib)

#Tests:
add_executable(eg test/cjson_test.c)
target_link_libraries(eg cjson)
