set(PROJECT_NAME "kommpot-tests")
project(${PROJECT_NAME} LANGUAGES CXX)

find_package(Threads REQUIRED)

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
include_directories(${CMAKE_CURRENT_LIST_DIR}/../libkommpot/include)
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
