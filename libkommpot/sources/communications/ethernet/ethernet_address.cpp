#include <communications/ethernet/ethernet_address.h>

#include <kommpot_core.h>

#include <cstdio>
#include <cstring>

ethernet_ipv4_address::ethernet_ipv4_address()
{
    value.fill(0x00);
}

auto ethernet_ipv4_address::to_uint32() const -> uint32_t
{
    return (static_cast<uint32_t>(value[0]) << 24) | (static_cast<uint32_t>(value[1]) << 16) |
           (static_cast<uint32_t>(value[2]) << 8) | static_cast<uint32_t>(value[3]);
}

auto ethernet_ipv4_address::to_string() const -> std::string
{
    char buffer[16] = {0};

    const int result = snprintf(buffer, sizeof(buffer), "%u.%u.%u.%u",
        static_cast<unsigned>(value[0]), static_cast<unsigned>(value[1]),
        static_cast<unsigned>(value[2]), static_cast<unsigned>(value[3]));
    if (result < 0)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to convert IPv4 address to string.");
        return "";
    }

    return std::string(buffer);
}

auto ethernet_ipv4_address::operator==(const ethernet_ipv4_address &other) const noexcept -> bool
{
    return value == other.value;
}

auto ethernet_ipv4_address::operator!=(const ethernet_ipv4_address &other) const noexcept -> bool
{
    return !(*this == other);
}

auto ethernet_ipv4_address::operator<(const ethernet_ipv4_address &other) const noexcept -> bool
{
    return to_uint32() < other.to_uint32();
}

auto ethernet_ipv4_address::operator<=(const ethernet_ipv4_address &other) const noexcept -> bool
{
    return to_uint32() <= other.to_uint32();
}

auto ethernet_ipv4_address::operator>(const ethernet_ipv4_address &other) const noexcept -> bool
{
    return to_uint32() > other.to_uint32();
}

auto ethernet_ipv4_address::operator>=(const ethernet_ipv4_address &other) const noexcept -> bool
{
    return to_uint32() >= other.to_uint32();
}

ethernet_ipv6_address::ethernet_ipv6_address()
{
    value.fill(0x00);
}

auto ethernet_ipv6_address::to_string() const -> std::string
{
    char buffer[40] = {0};

    const int result = snprintf(buffer, sizeof(buffer), "%x:%x:%x:%x:%x:%x:%x:%x",
        static_cast<unsigned>(value[0]), static_cast<unsigned>(value[1]),
        static_cast<unsigned>(value[2]), static_cast<unsigned>(value[3]),
        static_cast<unsigned>(value[4]), static_cast<unsigned>(value[5]),
        static_cast<unsigned>(value[6]), static_cast<unsigned>(value[7]));
    if (result < 0)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to convert IPv6 address to string.");
        return "";
    }

    return std::string(buffer);
}

auto ethernet_ipv6_address::operator==(const ethernet_ipv6_address &other) const noexcept -> bool
{
    return value == other.value;
}

auto ethernet_ipv6_address::operator!=(const ethernet_ipv6_address &other) const noexcept -> bool
{
    return !(*this == other);
}

auto ethernet_ipv6_address::operator<(const ethernet_ipv6_address &other) const noexcept -> bool
{
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] < other.value[i])
        {
            return true;
        }
        else if (value[i] > other.value[i])
        {
            return false;
        }
    }

    return false;
}

auto ethernet_ipv6_address::operator<=(const ethernet_ipv6_address &other) const noexcept -> bool
{
    return !(other < *this);
}

auto ethernet_ipv6_address::operator>(const ethernet_ipv6_address &other) const noexcept -> bool
{
    return !(*this <= other);
}

auto ethernet_ipv6_address::operator>=(const ethernet_ipv6_address &other) const noexcept -> bool
{
    return !(*this < other);
}

ethernet_mac_address::ethernet_mac_address()
{
    value.fill(0x00);
}

auto ethernet_mac_address::empty() const -> bool
{
    return value == std::array<uint8_t, 6>{};
}

auto ethernet_mac_address::to_string() const -> std::string
{
    char buffer[18] = {0};

    const int result = snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X", value[0],
        value[1], value[2], value[3], value[4], value[5]);
    if (result < 0)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to convert MAC address to string.");
        return "";
    }

    return std::string(buffer);
}

auto ethernet_mac_address::operator==(const ethernet_mac_address &other) const noexcept -> bool
{
    return value == other.value;
}

auto ethernet_mac_address::operator!=(const ethernet_mac_address &other) const noexcept -> bool
{
    return !(*this == other);
}

auto ethernet_mac_address::operator<(const ethernet_mac_address &other) const noexcept -> bool
{
    return value < other.value;
}

auto ethernet_mac_address::operator<=(const ethernet_mac_address &other) const noexcept -> bool
{
    return value <= other.value;
}

auto ethernet_mac_address::operator>(const ethernet_mac_address &other) const noexcept -> bool
{
    return value > other.value;
}

auto ethernet_mac_address::operator>=(const ethernet_mac_address &other) const noexcept -> bool
{
    return value >= other.value;
}
