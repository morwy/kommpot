#ifndef KOMMPOT_CORE_H
#define KOMMPOT_CORE_H

#pragma once

#include <libkommpot.h>
#include <spdlog/spdlog.h>

#include <future>

constexpr auto LOGGER_NAME = "kommpot";
#define KOMMPOT_LOGGER spdlog::get(LOGGER_NAME)

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

    auto initialize() -> bool;
    auto deinitialize() -> bool;

    auto settings() noexcept -> kommpot::settings_structure;
    auto set_settings(const kommpot::settings_structure &settings) noexcept -> void;

    auto devices(const std::vector<kommpot::device_identification> &identifications,
        kommpot::device_callback device_cb, kommpot::status_callback status_cb) -> void;

private:
    kommpot::settings_structure m_settings;
    std::shared_ptr<spdlog::logger> m_logger = nullptr;
    std::future<void> m_future;

    kommpot_core() = default;
    ~kommpot_core() = default;

    auto initialize_logger() -> void;
    auto deinitialize_logger() -> void;
};

#endif // KOMMPOT_CORE_H
