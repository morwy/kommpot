set(PROJECT_NAME "kommpot")
project(${PROJECT_NAME} LANGUAGES CXX C)

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
set(FILE_EXTENSIONS *.h *.hpp *.c *.cpp)
foreach(FILE_EXTENSION IN LISTS FILE_EXTENSIONS)
    file (GLOB_RECURSE FOUND_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${FILE_EXTENSION})
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${FOUND_FILES})
endforeach()

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
