cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME "kommpot-tests")
project(${PROJECT_NAME} LANGUAGES CXX)

find_package(Threads REQUIRED)

set(GOOGLETEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third-party/googletest")
if (NOT EXISTS ${GOOGLETEST_DIR})
    message(FATAL_ERROR "No googletest library found!")
endif ()

set(GOOGLETEST_SOURCES ${GOOGLETEST_DIR}/googletest)
set(GOOGLEMOCK_SOURCES ${GOOGLETEST_DIR}/googlemock)
set(GOOGLETEST_FILES ${GOOGLETEST_SOURCES}/src/gtest-all.cc)
set(GOOGLETEST_INCLUDES ${GOOGLETEST_SOURCES} ${GOOGLETEST_SOURCES}/include)
list(APPEND GOOGLETEST_FILES ${GOOGLEMOCK_SOURCES}/src/gmock-all.cc)
list(APPEND GOOGLETEST_INCLUDES ${GOOGLEMOCK_SOURCES} ${GOOGLEMOCK_SOURCES}/include)

include_directories(${GOOGLETEST_INCLUDES})

#
# Add test source files recursively.
#
set(FILE_EXTENSIONS *.h *.hpp *.c *.cpp)
foreach(FILE_EXTENSION IN LISTS FILE_EXTENSIONS)
    file (GLOB_RECURSE FOUND_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${FILE_EXTENSION})
    set(TEST_FOLDER_FILES ${TEST_FOLDER_FILES} ${FOUND_FILES})
endforeach()

#
# Add kommpot source files recursively.
#
include_directories(${CMAKE_CURRENT_LIST_DIR}/../kommpot/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party)

set(FILE_EXTENSIONS *.h *.hpp *.c *.cpp)
foreach(FILE_EXTENSION IN LISTS FILE_EXTENSIONS)
    file (GLOB_RECURSE FOUND_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../kommpot/${FILE_EXTENSION})
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${FOUND_FILES})
endforeach()

add_executable(${PROJECT_NAME} ${TEST_FOLDER_FILES} ${PROJECT_FOLDER_FILES} ${GOOGLETEST_FILES})

#
# Link with kommpot dependencies.
#
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads ${KOMMPOT_LIBRARIES})
foreach (KOMMPOT_DEPENDENCY IN LISTS KOMMPOT_DEPENDENCIES)
    add_dependencies(${PROJECT_NAME} ${KOMMPOT_DEPENDENCY})
endforeach()

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME} PROPERTIES LABELS "unit" DISCOVERY_TIMEOUT 240)
