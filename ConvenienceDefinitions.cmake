#
# Common definitions.
#
include(ProcessorCount)
ProcessorCount(CPU_CORE_COUNT)
message("CPU core count: ${CPU_CORE_COUNT}.")

if(NOT DEFINED CMAKE_BUILD_TYPE OR "${CMAKE_BUILD_TYPE}" STREQUAL "")
    message("CMake macro CMAKE_BUILD_TYPE was undefined, falling back to \"Release\".")
    set(CMAKE_BUILD_TYPE "Release")
endif()

message("Determined build options:")

#
# Operational system macro.
#
message("  - Build type: ${CMAKE_BUILD_TYPE}.")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    message("  - Operational system: Linux.")
    set(PLATFORM_OS_LINUX true)
    set(PLATFORM_OS "linux")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    message("  - Operational system: MacOS.")
    set(PLATFORM_OS_MACOS true)
    set(PLATFORM_OS "macos")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    message("  - Operational system: Windows.")
    set(PLATFORM_OS_WINDOWS true)
    set(PLATFORM_OS "windows")
else()
    message(FATAL_ERROR "  - Operational system: ${CMAKE_SYSTEM_NAME} (not supported).")
endif()

#
# Architecture bits.
#
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message("  - Platform bits: 32.")
    set(PLATFORM_BITS 32)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8 )
    message("  - Platform bits: 64.")
    set(PLATFORM_BITS 64)
else()
    message(FATAL_ERROR "  - Platform bitset: ${CMAKE_SIZEOF_VOID_P} (not supported).")
endif()

#
# Architecture type.
#
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch")
    if(PLATFORM_BITS EQUAL 32)
        set(PLATFORM_TYPE "arm32")
    elseif(PLATFORM_BITS EQUAL 64)
        set(PLATFORM_TYPE "arm64")
    endif()
    message("  - Platform: ${PLATFORM_TYPE}.")
else()
    if(PLATFORM_BITS EQUAL 32)
        set(PLATFORM_TYPE "x86")
    elseif(PLATFORM_BITS EQUAL 64)
        set(PLATFORM_TYPE "x86_64")
    endif()
    message("  - Platform: ${PLATFORM_TYPE}.")
endif()

message("")
