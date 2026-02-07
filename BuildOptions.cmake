#
# Tests.
#
option(IS_TESTING_ENABLED "Enable testing" OFF)

#
# Compilation type.
#
option(IS_COMPILING_STATIC "Enable libusb library" OFF)
option(IS_COMPILING_SHARED "Enable libusb library" ON)

#
# Libraries. They are injected into the code as preprocessor definitions in build_options.h.
#
option(IS_LIBUSB_ENABLED "Enable libusb library" ON)
option(IS_LIBFTDI_ENABLED "Enable libftdi library" ON)
option(IS_ETHERNET_ENABLED "Enable Ethernet library" ON)

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
message("")

configure_file(${CMAKE_CURRENT_LIST_DIR}/build_options.h.in ${CMAKE_CURRENT_LIST_DIR}/build_options.h)
