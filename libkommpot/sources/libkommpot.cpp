#include "libkommpot.h"

#include <utility>

kommpot::version::version(const uint8_t major, const uint8_t minor, const uint8_t micro,
                          const uint8_t nano, std::string git_hash)
    : m_major(major), m_minor(minor), m_micro(micro), m_nano(nano), m_git_hash(std::move(git_hash))
{
}

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

std::vector<kommpot::device_communication> kommpot::get_device_list(const int &device_id)
{
    return std::vector<kommpot::device_communication>();
}
