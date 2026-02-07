set(PROJECT_NAME "kommpot")
project(${PROJECT_NAME} LANGUAGES CXX C)

add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

#
# Set include libraries.
#
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/sources)
include_directories(${PROJECT_ROOT_PATH})
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party/spdlog/include/)

#
# Add source files recursively.
#
set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/include/export_definitions.h)
set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/include/libkommpot.h)

set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/sources/libkommpot.cpp)
set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/sources/kommpot_core.h)
set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/sources/kommpot_core.cpp)

if(IS_LIBUSB_ENABLED)
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/sources/communication_libusb.h)
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/sources/communication_libusb.cpp)
endif()

if(IS_LIBFTDI_ENABLED)
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/sources/communication_libftdi.h)
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/sources/communication_libftdi.cpp)
endif()

if(IS_ETHERNET_ENABLED)
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/sources/communications/ethernet/*)
endif()

if(IS_COMPILING_SHARED)
    set(LIBRARY_TYPE SHARED)
else()
    set(LIBRARY_TYPE STATIC)
endif()

add_library(${PROJECT_NAME} ${LIBRARY_TYPE} ${PROJECT_FOLDER_FILES})

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_definitions(${PROJECT_NAME} PRIVATE WIN_EXPORT)

    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        target_compile_options(${PROJECT_NAME} PRIVATE /MDd)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE /MD)
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${INTERNAL_LIBRARIES})
foreach (INTERNAL_DEPENDENCY IN LISTS INTERNAL_DEPENDENCIES)
    add_dependencies(${PROJECT_NAME} ${INTERNAL_DEPENDENCY})
endforeach()
