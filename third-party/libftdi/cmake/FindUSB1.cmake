# - Try to find the freetype library
# Once done this defines
#
#  LIBUSB_FOUND - system has libusb
#  LIBUSB_INCLUDE_DIR - the libusb include directory
#  LIBUSB_LIBRARIES - Link these to use libusb

# Copyright (c) 2006, 2008  Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if (LIBUSB_INCLUDE_DIR AND LIBUSB_LIBRARIES)

  # in cache already
  set(LIBUSB_FOUND TRUE)

else (LIBUSB_INCLUDE_DIR AND LIBUSB_LIBRARIES)
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  find_package(PkgConfig)
  pkg_check_modules(PC_LIBUSB libusb-1.0)

  FIND_PATH(LIBUSB_INCLUDE_DIR libusb.h
    PATH_SUFFIXES libusb-1.0
    PATHS ${PROJECT_ROOT_PATH}/third-party/libusb-cmake/libusb/libusb)

  #FIND_LIBRARY(LIBUSB_LIBRARIES NAMES usb-1.0
  #  PATHS ${PC_LIBUSB_LIBDIR} ${PC_LIBUSB_LIBRARY_DIRS})
  if(PLATFORM_OS_WINDOWS)
    set(LIBUSB_LIBRARIES ${BINARY_OUTPUT_PATH}/libusb-1.0.lib)
  elseif(PLATFORM_OS_MACOS)
    set(LIBUSB_LIBRARIES ${BINARY_OUTPUT_PATH}/libusb-1.0.dylib)
  endif()

  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBUSB DEFAULT_MSG LIBUSB_LIBRARIES LIBUSB_INCLUDE_DIR)

  MARK_AS_ADVANCED(LIBUSB_INCLUDE_DIR LIBUSB_LIBRARIES)

endif (LIBUSB_INCLUDE_DIR AND LIBUSB_LIBRARIES)
