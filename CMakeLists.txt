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
    message("Modifying BINARY_OUTPUT_PATH path under MSVC due to extra folders created automatically.")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH}/..)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH}/..)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH}/..)
else()
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH})
endif()

message("Creating ${BINARY_OUTPUT_PATH} build directory.")
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

if(PLATFORM_OS_MACOS)
    if(IS_COMPILING_SHARED)
        set(MACOSX_RPATH false)
        set(CMAKE_MACOSX_RPATH false)
        set(CMAKE_SKIP_RPATH false)
        #set(CMAKE_SKIP_BUILD_RPATH true)
    endif()
endif()

message("Compiling following ${COMPILATION_TYPE} dependencies:")

set(KOMMPOT_LIBRARIES "")
set(KOMMPOT_DEPENDENCIES kommpot)

set(INTERNAL_LIBRARIES "")
set(INTERNAL_DEPENDENCIES "")

if(IS_COMPILING_SHARED)
    if(PLATFORM_OS_LINUX)
        set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${BINARY_OUTPUT_PATH}/libkommpot.so)
    elseif(PLATFORM_OS_MACOS)
        set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${BINARY_OUTPUT_PATH}/libkommpot.dylib)
    elseif(PLATFORM_OS_WINDOWS)
        set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${BINARY_OUTPUT_PATH}/kommpot.lib)
    endif()
else()
    if(PLATFORM_OS_LINUX)
        set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${BINARY_OUTPUT_PATH}/libkommpot.a)
    elseif(PLATFORM_OS_MACOS)
        set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${BINARY_OUTPUT_PATH}/libkommpot.a)
    elseif(PLATFORM_OS_WINDOWS)
        set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${BINARY_OUTPUT_PATH}/kommpot.lib)
    endif()
endif()

if(IS_COMPILING_LIBUSB)
    if(IS_COMPILING_SHARED)
        set(LIBUSB_BUILD_SHARED_LIBS true)
    endif()

    set(LIBUSB_PROJECT_NAME "")
    if(PLATFORM_OS_LINUX)
        set(LIBUSB_PROJECT_NAME usb-1.0)
    elseif(PLATFORM_OS_MACOS)
        set(LIBUSB_PROJECT_NAME usb-1.0)
    elseif(PLATFORM_OS_WINDOWS)
        set(LIBUSB_PROJECT_NAME libusb-1.0)
    endif()
    
    set(INTERNAL_DEPENDENCIES ${INTERNAL_DEPENDENCIES} ${LIBUSB_PROJECT_NAME})

    if(IS_COMPILING_SHARED)
        if(PLATFORM_OS_LINUX)
            set(INTERNAL_LIBRARIES ${INTERNAL_LIBRARIES} ${BINARY_OUTPUT_PATH}/libusb-1.0.so)
        elseif(PLATFORM_OS_MACOS)
            set(INTERNAL_LIBRARIES ${INTERNAL_LIBRARIES} ${BINARY_OUTPUT_PATH}/libusb-1.0.dylib)
        elseif(PLATFORM_OS_WINDOWS)
            set(INTERNAL_LIBRARIES ${INTERNAL_LIBRARIES} ${BINARY_OUTPUT_PATH}/libusb-1.0.lib)
        endif()
    else()
        if(PLATFORM_OS_LINUX)
            set(INTERNAL_LIBRARIES ${INTERNAL_LIBRARIES} ${BINARY_OUTPUT_PATH}/libusb-1.0.a)
        elseif(PLATFORM_OS_MACOS)
            set(INTERNAL_LIBRARIES ${INTERNAL_LIBRARIES} ${BINARY_OUTPUT_PATH}/libusb-1.0.a)
        elseif(PLATFORM_OS_WINDOWS)
            set(INTERNAL_LIBRARIES ${INTERNAL_LIBRARIES} ${BINARY_OUTPUT_PATH}/libusb-1.0.lib)
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
        set(INTERNAL_DEPENDENCIES ${INTERNAL_DEPENDENCIES} ftdi1)

        if(PLATFORM_OS_LINUX)
            set(INTERNAL_LIBRARIES ${INTERNAL_LIBRARIES} ${BINARY_OUTPUT_PATH}/libftdi1.so.2.5.0)
        elseif(PLATFORM_OS_MACOS)
            set(INTERNAL_LIBRARIES ${INTERNAL_LIBRARIES} ${BINARY_OUTPUT_PATH}/libftdi1.2.5.0.dylib)
        elseif(PLATFORM_OS_WINDOWS)
            set(INTERNAL_LIBRARIES ${INTERNAL_LIBRARIES} ${BINARY_OUTPUT_PATH}/ftdi1.lib)
        endif()
    else()
        set(INTERNAL_DEPENDENCIES ${INTERNAL_DEPENDENCIES} ftdi1-static)

        if(PLATFORM_OS_LINUX)
            set(INTERNAL_LIBRARIES ${INTERNAL_LIBRARIES} ${BINARY_OUTPUT_PATH}/libftdi1.a)
        elseif(PLATFORM_OS_MACOS)
            set(INTERNAL_LIBRARIES ${INTERNAL_LIBRARIES} ${BINARY_OUTPUT_PATH}/libftdi1.a)
        elseif(PLATFORM_OS_WINDOWS)
            set(INTERNAL_LIBRARIES ${INTERNAL_LIBRARIES} ${BINARY_OUTPUT_PATH}/ftdi1.lib)
        endif()
    endif()

    message("  - libftdi")
    add_subdirectory(third-party/libftdi)
endif()

message("INTERNAL_DEPENDENCIES: ${INTERNAL_DEPENDENCIES}")
message("INTERNAL_LIBRARIES: ${INTERNAL_LIBRARIES}")

set(KOMMPOT_DEPENDENCIES ${KOMMPOT_DEPENDENCIES} ${INTERNAL_DEPENDENCIES})
set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} ${INTERNAL_LIBRARIES})

if(PLATFORM_OS_LINUX)
    set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} udev)
elseif(PLATFORM_OS_MACOS)
    set(KOMMPOT_LIBRARIES ${KOMMPOT_LIBRARIES} "-framework IOKit" "-framework CoreFoundation" "-framework Security" objc)
endif()

message("KOMMPOT_DEPENDENCIES: ${KOMMPOT_DEPENDENCIES}")
message("KOMMPOT_LIBRARIES: ${KOMMPOT_LIBRARIES}")

message("")

#
# State global include libraries.
#
include_directories(${PROJECT_ROOT_PATH})
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party/spdlog/include/)

#
# Add projects.
#
add_subdirectory(libkommpot)

#
# Add tests.
#
enable_testing()
add_subdirectory(tests)
