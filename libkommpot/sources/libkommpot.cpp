#include "libkommpot.h"

#include <communication_libusb.h>
#include <communications/ethernet/communication_ethernet.h>
#include <kommpot_core.h>

#include <cstdint>
#include <iterator>
#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>

auto kommpot::initialize() -> bool
{
    return kommpot_core::instance().initialize();
}

auto kommpot::deinitialize() -> bool
{
    return kommpot_core::instance().deinitialize();
}

auto kommpot::settings() noexcept -> kommpot::settings_structure
{
    return kommpot_core::instance().settings();
}

auto kommpot::set_settings(const settings_structure &settings) noexcept -> void
{
    kommpot_core::instance().set_settings(settings);
}

kommpot::version::version(const uint8_t major, const uint8_t minor, const uint8_t micro,
    const uint8_t nano, std::string git_hash)
    : m_major(major)
    , m_minor(minor)
    , m_micro(micro)
    , m_nano(nano)
    , m_git_hash(std::move(git_hash))
{}

auto kommpot::version::major() const noexcept -> uint8_t
{
    return m_major;
}

auto kommpot::version::minor() const noexcept -> uint8_t
{
    return m_minor;
}

auto kommpot::version::micro() const noexcept -> uint8_t
{
    return m_micro;
}

auto kommpot::version::nano() const noexcept -> uint8_t
{
    return m_nano;
}

auto kommpot::version::git_hash() const noexcept -> std::string
{
    return m_git_hash;
}

auto kommpot::version::to_string() const noexcept -> std::string
{
    return std::to_string(m_major) + "." + std::to_string(m_minor) + "." + std::to_string(m_micro) +
           "." + std::to_string(m_nano);
}

auto kommpot::get_version() noexcept -> kommpot::version
{
    return {PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_MICRO,
        PROJECT_VERSION_NANO, PROJECT_VERSION_SHA};
}

auto kommpot::communication_type_to_string(const communication_type &type) noexcept -> std::string
{
    switch (type)
    {
    case communication_type::UNKNOWN: {
        return "Unknown";
    }
    case communication_type::LIBUSB: {
        return "libusb";
    }
    case communication_type::LIBFTDI: {
        return "libftdi";
    }
    case communication_type::ETHERNET: {
        return "ethernet";
    }
    default:
        return "";
    }
}

auto kommpot::ethernet_protocol_type_to_string(const ethernet_protocol_type &type) noexcept
    -> std::string
{
    switch (type)
    {
    case ethernet_protocol_type::UNKNOWN: {
        return "UNKNOWN";
    }
    case ethernet_protocol_type::TCP: {
        return "TCP";
    }
    case ethernet_protocol_type::UDP: {
        return "UDP";
    }
    default:
        return "";
    }
}

kommpot::device_communication::device_communication(kommpot::device_identification identification)
    : m_identification_variant(std::move(identification))
{}

auto kommpot::device_communication::type() const -> kommpot::communication_type
{
    return m_type;
}

auto kommpot::devices(const std::vector<device_identification> &identifications)
    -> std::vector<std::shared_ptr<kommpot::device_communication>>
{
    std::vector<std::shared_ptr<kommpot::device_communication>> device_list;

    /**
     * @brief libusb devices.
     */
    auto libusb_devices = communication_libusb::devices(identifications);
    device_list.insert(std::end(device_list), std::make_move_iterator(std::begin(libusb_devices)),
        std::make_move_iterator(std::end(libusb_devices)));

    /**
     * @brief ethernet devices.
     */
    auto ethernet_devices = communication_ethernet::devices(identifications);
    device_list.insert(std::end(device_list), std::make_move_iterator(std::begin(ethernet_devices)),
        std::make_move_iterator(std::end(ethernet_devices)));

    return device_list;
}

auto kommpot::devices(const std::vector<device_identification> &identifications,
    device_callback device_cb, status_callback status_cb) -> void
{
    kommpot_core::instance().devices(identifications, device_cb, status_cb);
}
