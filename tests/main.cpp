#include <gtest/gtest.h>

#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/spdlog.h>

auto main(int argc, char *argv[]) -> int
{
    // Register the "kommpot" logger so that KOMMPOT_LOGGER (spdlog::get("kommpot"))
    // does not return nullptr when factory error paths are exercised.
    if (!spdlog::get("kommpot"))
    {
        auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
        auto logger = std::make_shared<spdlog::logger>("kommpot", sink);
        spdlog::register_logger(logger);
    }

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}