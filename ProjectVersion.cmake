#
# Stating common version definitions.
#
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_MICRO 4)
set(PROJECT_VERSION_NANO 0)
set(PROJECT_VERSION_SHA "0")

#
# Reading Git SHA value.
#
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE GIT_CALL_RESULT
                    OUTPUT_VARIABLE PROJECT_VERSION_SHA
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT GIT_CALL_RESULT EQUAL "0")
        message(FATAL_ERROR "\"${GIT_EXECUTABLE} rev-parse --short HEAD\"¨ failed with ${GIT_CALL_RESULT}")
    endif()
else()
    message(FATAL_ERROR "Git executable is not found, please install it.")
endif()

#
# Passing definitions into project sources.
#
add_definitions(-DPROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR} -DPROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR} -DPROJECT_VERSION_MICRO=${PROJECT_VERSION_MICRO} -DPROJECT_VERSION_NANO=${PROJECT_VERSION_NANO} -DPROJECT_VERSION_SHA="${PROJECT_VERSION_SHA}")

message("Project version: ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_MICRO}.${PROJECT_VERSION_NANO}.${PROJECT_VERSION_SHA}")

message("")
