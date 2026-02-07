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
file(GLOB INCLUDE_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/include/*)
set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${INCLUDE_FILES})

file(GLOB SOURCES_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/sources/*)
set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${SOURCES_FILES})

if(IS_LIBUSB_ENABLED)
    file(GLOB LIBUSB_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/sources/communications/libusb/*)
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${LIBUSB_FILES})
endif()

if(IS_LIBFTDI_ENABLED)
    file(GLOB LIBFTDI_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/sources/communications/libftdi/*)
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${LIBFTDI_FILES})
endif()

if(IS_ETHERNET_ENABLED)
    file(GLOB ETHERNET_FILES ${PROJECT_FOLDER_FILES} ${CMAKE_CURRENT_LIST_DIR}/sources/communications/ethernet/*)
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${ETHERNET_FILES})
endif()

message("Source files for ${PROJECT_NAME}:")
foreach (SOURCE_FILE IN LISTS PROJECT_FOLDER_FILES)
    message("  - ${SOURCE_FILE}")
endforeach()
message("")

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
