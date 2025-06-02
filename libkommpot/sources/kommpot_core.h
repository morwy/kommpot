#ifndef KOMMPOT_CORE_H
#define KOMMPOT_CORE_H

#pragma once

#include <libkommpot.h>
#include <spdlog/spdlog.h>

constexpr auto LOGGER_NAME = "kommpot";

#define KOMMPOT_LOGGER spdlog::get(LOGGER_NAME)

#define LOG_TRACE (KOMMPOT_LOGGER)->trace
#define LOG_DEBUG (KOMMPOT_LOGGER)->debug
#define LOG_INFO (KOMMPOT_LOGGER)->info
#define LOG_WARN (KOMMPOT_LOGGER)->warn
#define LOG_ERROR (KOMMPOT_LOGGER)->error
#define LOG_CRITICAL (KOMMPOT_LOGGER)->critical

class kommpot_core
{
public:
    static auto instance() -> kommpot_core &
    {
        static kommpot_core instance;
        return instance;
    }

    kommpot_core(const kommpot_core &) = delete;
    auto operator=(const kommpot_core &) -> void = delete;

    auto settings() noexcept -> kommpot::settings_structure;
    auto set_settings(const kommpot::settings_structure &settings) noexcept -> void;

private:
    kommpot::settings_structure m_settings;
    std::shared_ptr<spdlog::logger> m_logger;

    kommpot_core();
    ~kommpot_core();

    auto initialize_logger() -> void;
    auto deinitialize_logger() -> void;
};

#endif // KOMMPOT_CORE_H
