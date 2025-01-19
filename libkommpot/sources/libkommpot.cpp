#include "libkommpot.h"

#include "communication_libusb.h"

#include <cstdint>
#include <iterator>
#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>

namespace kommpot {
static spdlog_initializer gs_initializer;
}

kommpot::spdlog_initializer::spdlog_initializer()
{
    static auto console = spdlog::stdout_color_mt("libkommpot");
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
    default:
        return "";
    }
}

kommpot::device_communication::device_communication(communication_information information)
    : m_information(std::move(information))
{}

auto kommpot::device_communication::type() const -> kommpot::communication_type
{
    return m_type;
}

auto kommpot::devices(const std::vector<device_identification> &identifications)
    -> std::vector<std::unique_ptr<kommpot::device_communication>>
{
    std::vector<std::unique_ptr<kommpot::device_communication>> device_list;

    /**
     * @brief libusb devices.
     */
    auto libusb_devices = communication_libusb::devices(identifications);
    device_list.insert(std::end(device_list), std::make_move_iterator(std::begin(libusb_devices)),
        std::make_move_iterator(std::end(libusb_devices)));

    return device_list;
}
