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
set(BINARY_OUTPUT_PATH ${CMAKE_CURRENT_LIST_DIR}/bin/${PLATFORM_OS}_${PLATFORM_TYPE}/${CMAKE_BUILD_TYPE})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH})

file(MAKE_DIRECTORY ${BINARY_OUTPUT_PATH})

#
# Add projects.
#
