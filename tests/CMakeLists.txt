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
include_directories(${CMAKE_CURRENT_LIST_DIR}/../libkommpot/sources)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../third-party/spdlog/include/)

set(FILE_EXTENSIONS *.h *.hpp *.c *.cpp)
foreach(FILE_EXTENSION IN LISTS FILE_EXTENSIONS)
    file (GLOB_RECURSE FOUND_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../kommpot/${FILE_EXTENSION})
    set(PROJECT_FOLDER_FILES ${PROJECT_FOLDER_FILES} ${FOUND_FILES})
endforeach()

add_executable(${PROJECT_NAME} ${TEST_FOLDER_FILES} ${PROJECT_FOLDER_FILES})

target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)

#
# Link with kommpot dependencies.
#
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads GTest::gtest GTest::gmock ${KOMMPOT_LIBRARIES})

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()

foreach (KOMMPOT_DEPENDENCY IN LISTS KOMMPOT_DEPENDENCIES)
    add_dependencies(${PROJECT_NAME} ${KOMMPOT_DEPENDENCY})
endforeach()

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME} PROPERTIES LABELS "unit" DISCOVERY_TIMEOUT 240)

#
# Run tests on every build.
#
add_custom_target(check ALL
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ${PROJECT_NAME}
)
