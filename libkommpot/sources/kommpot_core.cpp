#include "kommpot_core.h"

#include <iostream>
#include <spdlog/async.h>
#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#    include <spdlog/sinks/msvc_sink.h>
#endif

#ifdef __linux__
#    include <spdlog/sinks/syslog_sink.h>
#endif

#ifdef __APPLE__
#    include <spdlog/sinks/syslog_sink.h>
#endif

kommpot_core::kommpot_core() {}

kommpot_core::~kommpot_core()
{
    deinitialize_logger();
}

auto kommpot_core::settings() noexcept -> kommpot::settings_structure
{
    return m_settings;
}

auto kommpot_core::set_settings(const kommpot::settings_structure &settings) noexcept -> void
{
    m_settings = settings;
    initialize_logger();
}

auto kommpot_core::initialize_logger() -> void
{
    std::vector<spdlog::sink_ptr> new_sinks;

    if (m_settings.logging_callback != nullptr)
    {
        /**
         * @brief initialize only callback functionality.
         */
        auto callback_sink = std::make_shared<spdlog::sinks::callback_sink_mt>(
            [](const spdlog::details::log_msg &msg) {
                const auto &settings = kommpot_core::instance().settings();
                if (settings.logging_callback != nullptr)
                {
                    settings.logging_callback(kommpot::callback_response_structure{
                        kommpot::logging_level(msg.level), msg.source.filename, msg.source.line,
                        msg.source.funcname, std::string(msg.payload.data(), msg.payload.size())});
                }
                else
                {
                    std::cerr << "Logging callback is null!" << std::endl;
                }
            });
        new_sinks.push_back(callback_sink);
    }
    else
    {
        /**
         * @brief initialize default functionality.
         */
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        new_sinks.push_back(console_sink);

#ifdef _WIN32
        auto msvc_qt_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
        new_sinks.push_back(msvc_qt_sink);
#endif

#ifdef __linux__
        auto syslog_sink =
            std::make_shared<spdlog::sinks::syslog_sink_mt>("kommpot", 0, LOG_USER, false);
        new_sinks.push_back(syslog_sink);
#endif

#ifdef __APPLE__
        auto syslog_sink =
            std::make_shared<spdlog::sinks::syslog_sink_mt>("kommpot", 0, LOG_USER, false);
        new_sinks.push_back(syslog_sink);
#endif
    }

    if (m_logger == nullptr)
    {
        m_logger =
            std::make_shared<spdlog::logger>(LOGGER_NAME, new_sinks.begin(), new_sinks.end());
        spdlog::register_logger(m_logger);
    }
    else
    {
        auto &sinks = KOMMPOT_LOGGER->sinks();
        sinks.clear();
        sinks = {new_sinks.begin(), new_sinks.end()};
    }

    KOMMPOT_LOGGER->set_level(spdlog::level::level_enum(m_settings.logging_level));
    KOMMPOT_LOGGER->set_pattern(m_settings.logging_pattern);

    LOG_DEBUG("A new logging session is started.");
}

auto kommpot_core::deinitialize_logger() -> void
{
    LOG_DEBUG("The logging session is finished.");
    KOMMPOT_LOGGER->flush();
    spdlog::drop(LOGGER_NAME);
}
