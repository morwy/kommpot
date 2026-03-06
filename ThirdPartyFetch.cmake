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

    enable_testing()

    message("GoogleTest test library was fetched to directory: ${googletest_SOURCE_DIR}.")
endif()
