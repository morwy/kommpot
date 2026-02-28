#
# Tests.
#
option(IS_TESTING_ENABLED "Enable testing" OFF)

#
# Compilation type.
#
option(IS_COMPILING_STATIC "Enable static library compilation" OFF)
option(IS_COMPILING_SHARED "Enable shared library compilation" ON)

if (NOT IS_COMPILING_STATIC AND NOT IS_COMPILING_SHARED)
    message(FATAL_ERROR "At least one of the library types (static or shared) must be enabled.")
endif()

#
# Libraries. They are injected into the code as preprocessor definitions in build_options.h.
#
option(IS_LIBUSB_ENABLED "Enable libusb library" ON)
option(IS_LIBFTDI_ENABLED "Enable libftdi library" OFF)
option(IS_ETHERNET_ENABLED "Enable Ethernet library" ON)
option(IS_HTTP_ENABLED "Enable HTTP library" ON)

if (NOT IS_LIBUSB_ENABLED AND NOT IS_LIBFTDI_ENABLED AND NOT IS_ETHERNET_ENABLED AND NOT IS_HTTP_ENABLED)
    message(FATAL_ERROR "At least one of the communication libraries (libusb, libftdi, Ethernet, HTTP) must be enabled.")
endif()

if (IS_HTTP_ENABLED AND NOT IS_ETHERNET_ENABLED)
    message("HTTP library depends on Ethernet library, enabling it.")
    set(IS_ETHERNET_ENABLED ON)
endif()

#
# Log.
#
message("Determined kommpot build options:")
message("  - Compiling static libraries: ${IS_COMPILING_STATIC}.")
message("  - Compiling shared libraries: ${IS_COMPILING_SHARED}.")
message("  - Compiling tests: ${IS_TESTING_ENABLED}.")
message("  - Using libusb: ${IS_LIBUSB_ENABLED}.")
message("  - Using libftdi: ${IS_LIBFTDI_ENABLED}.")
message("  - Using Ethernet: ${IS_ETHERNET_ENABLED}.")
message("  - Using HTTP: ${IS_HTTP_ENABLED}.")
message("")

configure_file(${CMAKE_CURRENT_LIST_DIR}/build_options.h.in ${CMAKE_CURRENT_LIST_DIR}/build_options.h)
