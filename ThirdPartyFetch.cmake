if(IS_TESTING_ENABLED)
    message("Fetching GoogleTest test library.")

    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/releases/download/v1.16.0/googletest-1.16.0.tar.gz
    )

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    if (EXISTS ${googletest_SOURCE_DIR})
        set(GTEST_SOURCES ${googletest_SOURCE_DIR}/googletest)
        set(GMOCK_SOURCES ${googletest_SOURCE_DIR}/googlemock)
    elseif (UNIX AND EXISTS /usr/src/gtest)
        set(GTEST_SOURCES /usr/src/gtest)
        message(WARNING "Using gtest from system")
        if (EXISTS /usr/src/gmock)
            set(GMOCK_SOURCES /usr/src/gmock)
        endif ()
    else ()
        message(FATAL_ERROR "No googletest src dir found - set googletest_SOURCE_DIR to enable!")
    endif ()

    set(GTEST_FILES ${GTEST_SOURCES}/src/gtest-all.cc)
    set(GTEST_INCLUDES ${GTEST_SOURCES} ${GTEST_SOURCES}/include)
    if (NOT ${GMOCK_SOURCES} STREQUAL "")
        list(APPEND GTEST_FILES ${GMOCK_SOURCES}/src/gmock-all.cc)
        list(APPEND GTEST_INCLUDES ${GMOCK_SOURCES} ${GMOCK_SOURCES}/include)
    endif ()

    include_directories(${GTEST_INCLUDES})

    enable_testing()

    message("GoogleTest test library was fetched to directory: ${googletest_SOURCE_DIR}.")
endif()
