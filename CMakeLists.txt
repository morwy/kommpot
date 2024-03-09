#
# Required CMake version.
#
cmake_minimum_required(VERSION 3.19)
cmake_policy(VERSION 3.10)

project(kommpot LANGUAGES CXX)

#
# Required C++ standard.
#
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Project version.
#
include(ProjectVersion.cmake)

#
# Convenience definitions.
#
include(ConvenienceDefinitions.cmake)

#
# Set output folder for binaries.
#
set(PROJECT_ROOT_PATH ${CMAKE_CURRENT_LIST_DIR})
set(BINARY_OUTPUT_PATH ${PROJECT_ROOT_PATH}/bin/${PLATFORM_OS}_${PLATFORM_TYPE}/${CMAKE_BUILD_TYPE})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH})

file(MAKE_DIRECTORY ${BINARY_OUTPUT_PATH})

#
# State which dependencies to compile.
#
set(IS_COMPILING_LIBUSB true)

#
# Add dependencies.
# Create list of built dependencies and their include directories.
#
message("Compiling following dependencies:")

if(IS_COMPILING_LIBUSB)
    message("  - libusb")
    add_subdirectory(third-party/libusb-cmake)
endif()

message("")

#
# Add projects.
#
add_subdirectory(libkommpot)
