set(PROJECT_NAME "kommpot")
project(${PROJECT_NAME} LANGUAGES CXX C)

#
# Set include libraries.
#
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/sources)
include_directories(${PROJECT_ROOT_PATH}/third-party)

#
# Add source files recursively.
#
set(FILE_EXTENSIONS *.h *.hpp *.c *.cpp)
foreach(FILE_EXTENSION IN LISTS FILE_EXTENSIONS)
    file (GLOB_RECURSE FOUND_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${FILE_EXTENSION})
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${FOUND_FILES})
endforeach()

if(IS_COMPILING_SHARED)
    set(LIBRARY_TYPE SHARED)
    set(MSVC_COMPILATION_PARAMETER "/MD")
else()
    set(LIBRARY_TYPE STATIC)
    set(MSVC_COMPILATION_PARAMETER "/MD")
endif()

add_library(${PROJECT_NAME} ${LIBRARY_TYPE} ${PROJECT_FOLDER_FILES})

target_compile_definitions(${PROJECT_NAME} PRIVATE LIBRARY_EXPORT)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /EHsc ${MSVC_COMPILATION_PARAMETER} /c)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WIN_EXPORT)
endif()