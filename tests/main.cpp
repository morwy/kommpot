#include <gtest/gtest.h>

#ifdef _WIN32
#    include <spdlog/sinks/msvc_sink.h>
#else
#    include <spdlog/sinks/stdout_color_sinks.h>
#endif
#include <spdlog/spdlog.h>

auto main(int argc, char *argv[]) -> int
{
    // Register the "kommpot" logger so that KOMMPOT_LOGGER (spdlog::get("kommpot"))
    // does not return nullptr when factory error paths are exercised.
    if (!spdlog::get("kommpot"))
    {
#ifdef _WIN32
        auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
#endif
        auto logger = std::make_shared<spdlog::logger>("kommpot", sink);
        spdlog::register_logger(logger);
    }

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}