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

if(MSVC)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH}/..)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH}/..)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH}/..)
else()
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH})
endif()

file(MAKE_DIRECTORY ${BINARY_OUTPUT_PATH})

#
# Define compilation settings.
#
if(NOT DEFINED IS_COMPILING_STATIC AND NOT DEFINED IS_COMPILING_SHARED)
    set(IS_COMPILING_STATIC false)
    set(IS_COMPILING_SHARED true)
endif()

if(NOT DEFINED IS_COMPILING_LIBUSB)
    set(IS_COMPILING_LIBUSB true)
endif()

if(NOT DEFINED IS_COMPILING_LIBFTDI)
    set(IS_COMPILING_LIBFTDI true)
endif()

#
# Add dependencies.
# Create list of built dependencies and their include directories.
#
if(IS_COMPILING_STATIC)
    set(COMPILATION_TYPE "static")
else()
    set(COMPILATION_TYPE "shared")
endif()

message("Compiling following ${COMPILATION_TYPE} dependencies:")

set(KOMMPOT_LIBRARIES "")
set(KOMMPOT_DEPENDENCIES ${KOMMPOT_DEPENDENCIES} kommpot)

if(IS_COMPILING_SHARED)
    if(PLATFORM_OS_LINUX)
        set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${BINARY_OUTPUT_PATH}/libkommpot.so)
    elseif(PLATFORM_OS_WINDOWS)
        set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${BINARY_OUTPUT_PATH}/libkommpot.dll)
    endif()
else()
    if(PLATFORM_OS_LINUX)
        set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${BINARY_OUTPUT_PATH}/libkommpot.a)
    elseif(PLATFORM_OS_WINDOWS)
        set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${BINARY_OUTPUT_PATH}/libkommpot.lib)
    endif()
endif()

if(IS_COMPILING_LIBUSB)
    if(IS_COMPILING_SHARED)
        set(LIBUSB_BUILD_SHARED_LIBS true)
    endif()

    set(LIBUSB_PROJECT_NAME "")
    if(PLATFORM_OS_LINUX)
        set(LIBUSB_PROJECT_NAME usb-1.0)
    elseif(PLATFORM_OS_WINDOWS)
        set(LIBUSB_PROJECT_NAME libusb-1.0)
    endif()
    
    set(KOMMPOT_DEPENDENCIES ${KOMMPOT_DEPENDENCIES} ${LIBUSB_PROJECT_NAME})

    if(IS_COMPILING_SHARED)
        if(PLATFORM_OS_LINUX)
            set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${BINARY_OUTPUT_PATH}/libusb-1.0.so)
        elseif(PLATFORM_OS_WINDOWS)
            set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${BINARY_OUTPUT_PATH}/libusb-1.0.dll)
        endif()
    else()
        if(PLATFORM_OS_LINUX)
            set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${BINARY_OUTPUT_PATH}/libusb-1.0.a)
        elseif(PLATFORM_OS_WINDOWS)
            set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${BINARY_OUTPUT_PATH}/libusb-1.0.lib)
        endif()
    endif()

    message("  - libusb")
    add_subdirectory(third-party/libusb-cmake)
endif()

if(IS_COMPILING_LIBFTDI)
    if(NOT IS_COMPILING_LIBUSB)
        message(FATAL_ERROR "libusb is required for libftdi!")
    endif()

    if(IS_COMPILING_SHARED)
        set(KOMMPOT_DEPENDENCIES ${KOMMPOT_DEPENDENCIES} ftdi1)

        if(PLATFORM_OS_LINUX)
            set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${BINARY_OUTPUT_PATH}/libftdi1.so.2.5.0)
        elseif(PLATFORM_OS_WINDOWS)
            set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${BINARY_OUTPUT_PATH}/ftdi1.dll)
        endif()
    else()
        set(KOMMPOT_DEPENDENCIES ${KOMMPOT_DEPENDENCIES} ftdi1-static)

        if(PLATFORM_OS_LINUX)
            set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${BINARY_OUTPUT_PATH}/libftdi1.a.2.5.0)
        elseif(PLATFORM_OS_WINDOWS)
            set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${BINARY_OUTPUT_PATH}/ftdi1.lib)
        endif()
    endif()

    message("  - libftdi")
    add_subdirectory(third-party/libftdi)
endif()

message("KOMMPOT_DEPENDENCIES: ${KOMMPOT_DEPENDENCIES}")
message("KOMMPOT_LIBRARIES: ${KOMMPOT_LIBRARIES}")

message("")

#
# Add projects.
#
add_subdirectory(libkommpot)
