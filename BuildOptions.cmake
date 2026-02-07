#
# Tests.
#
option(IS_TESTING_ENABLED "Enable testing" OFF)

#
# Libraries. They are injected into the code as preprocessor definitions in build_options.h.
#
option(IS_LIBUSB_ENABLED "Enable libusb library" ON)
option(IS_LIBFTDI_ENABLED "Enable libftdi library" ON)
option(IS_ETHERNET_ENABLED "Enable Ethernet library" ON)

configure_file(${CMAKE_CURRENT_LIST_DIR}/build_options.h.in ${CMAKE_CURRENT_LIST_DIR}/build_options.h)
