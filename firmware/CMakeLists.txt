cmake_minimum_required(VERSION 3.16)
set(IDF_TARGET "esp32s3" CACHE STRING "Prototype target")

# load the ESP-IDF CMake build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# define the firmware project name
project(iot_visual_management_station)
