cmake_minimum_required(VERSION 3.10.2)

#include the header files directorire of libraries
include_directories(cjson)

set(cjson_srcs 
    cjson/cJSON.c
)

add_library(cjson SHARED ${cjson_srcs})
