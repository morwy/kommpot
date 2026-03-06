#include <communications/ethernet/ethernet_address_factory.h>

#include <kommpot_core.h>

#include <charconv>
#include <limits>
#include <optional>
#include <string_view>

auto ethernet_address_factory::from_string(const std::string &value)
    -> std::optional<std::shared_ptr<ethernet_ip_address>>
{
    auto pos = value.find('\0');
    if (pos != std::string::npos && pos != (value.size() - 1))
    {
        SPDLOG_LOGGER_ERROR(
            KOMMPOT_LOGGER, "Invalid IP address: {}, contains embedded null character.", value);
        return std::nullopt;
    }

    /**
     * Checking for IPv6 first, as it can contain dots as part of the address (e.g., in IPv4-mapped
     * IPv6 addresses), while IPv4 addresses cannot contain colons. This way we can route the input
     * string to the correct parser based on its format.
     */
    if (value.find(':') != std::string::npos)
    {
        auto ip = std::make_shared<ethernet_ipv6_address>();

        struct in6_addr bytes = {};
        if (inet_pton(AF_INET6, value.c_str(), &bytes) != 1)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Invalid IPv6 address: {}.", value);
            return std::nullopt;
        }

        for (size_t i = 0; i < ip->value.size(); ++i)
        {
            ip->value[i] =
                (static_cast<uint16_t>(bytes.s6_addr[2 * i]) << 8) | bytes.s6_addr[2 * i + 1];
        }

        return ip;
    }
    else if (value.find('.') != std::string::npos)
    {
        size_t index = 0;

        if (value.front() == '.' || value.back() == '.')
        {
            SPDLOG_LOGGER_ERROR(
                KOMMPOT_LOGGER, "Invalid IPv4 address: {}, cannot start or end with a dot.", value);
            return std::nullopt;
        }

        auto ip = std::make_shared<ethernet_ipv4_address>();

        std::string_view remaining(value);
        while (!remaining.empty())
        {
            if (index >= ip->value.size())
            {
                SPDLOG_LOGGER_ERROR(
                    KOMMPOT_LOGGER, "Invalid IPv4 address: {}, too many octets.", value);
                return std::nullopt;
            }

            const auto dot_pos = remaining.find('.');
            const auto token = remaining.substr(0, dot_pos);
            remaining = (dot_pos == std::string_view::npos) ? std::string_view{}
                                                            : remaining.substr(dot_pos + 1);

            if (token.empty())
            {
                SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                    "Invalid IPv4 address: {}, empty octet detected (consecutive dots).", value);
                return std::nullopt;
            }

            const bool is_digits = std::all_of(
                token.begin(), token.end(), [](unsigned char c) { return std::isdigit(c); });
            if (!is_digits)
            {
                SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                    "Invalid IPv4 address: {}, non-digit characters detected in octet.", value);
                return std::nullopt;
            }

            if (token.size() > 1 && token[0] == '0')
            {
                SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                    "Invalid IPv4 address: {}, leading zeros are not permitted in octet.", value);
                return std::nullopt;
            }

            unsigned int digit = {};

            const auto result = std::from_chars(token.data(), token.data() + token.size(), digit);
            if (result.ec != std::errc() || digit > 255)
            {
                SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                    "Invalid IPv4 address: {}, failed with error '{}'.", value,
                    std::make_error_code(result.ec).message());
                return std::nullopt;
            }

            ip->value.at(index++) = static_cast<uint8_t>(digit);
        }

        if (index != ip->value.size())
        {
            SPDLOG_LOGGER_ERROR(
                KOMMPOT_LOGGER, "Invalid IPv4 address: {}, too few octets ({}).", value, index);
            return std::nullopt;
        }

        return ip;
    }

    SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided IP address is not IPv4 or IPv6.");

    return std::nullopt;
}

auto ethernet_address_factory::from_uint32_t(const uint32_t value)
    -> std::optional<std::shared_ptr<ethernet_ip_address>>
{
    auto ip = std::make_shared<ethernet_ipv4_address>();

    ip->value.at(0) = static_cast<uint8_t>((value >> 24) & 0xFF);
    ip->value.at(1) = static_cast<uint8_t>((value >> 16) & 0xFF);
    ip->value.at(2) = static_cast<uint8_t>((value >> 8) & 0xFF);
    ip->value.at(3) = static_cast<uint8_t>(value & 0xFF);

    return ip;
}

auto ethernet_address_factory::from_sockaddr_in(const sockaddr *structure)
    -> std::optional<std::shared_ptr<ethernet_ip_address>>
{
    if (structure == nullptr)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided sockaddr structure is nullptr.");
        return std::nullopt;
    }

    if (structure->sa_family == AF_INET)
    {
        auto ip = std::make_shared<ethernet_ipv4_address>();

        const sockaddr_in *ipv4 = reinterpret_cast<const sockaddr_in *>(structure);
        std::memcpy(ip->value.data(), &ipv4->sin_addr, ip->value.size());

        return ip;
    }
    else if (structure->sa_family == AF_INET6)
    {
        auto ip = std::make_shared<ethernet_ipv6_address>();

        const sockaddr_in6 *ipv6 = reinterpret_cast<const sockaddr_in6 *>(structure);
        for (size_t segment_index = 0; segment_index < ip->value.size(); ++segment_index)
        {
            uint16_t segment = 0;
            std::memcpy(&segment, &ipv6->sin6_addr.s6_addr[segment_index * 2], 2);
            ip->value.at(segment_index) = ntohs(segment);
        }

        return ip;
    }

    SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided IP address is not IPv4 or IPv6.");

    return std::nullopt;
}

auto ethernet_address_factory::from_array(const uint8_t *ptr, const size_t length)
    -> std::optional<ethernet_mac_address>
{
    if (ptr == nullptr)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Invalid input for MAC address: pointer is nullptr.");
        return std::nullopt;
    }

    ethernet_mac_address address;

    if (length < address.value.size())
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
            "Invalid input for MAC address: length {} is less than required {}.", length,
            address.value.size());
        return std::nullopt;
    }

    std::memcpy(address.value.data(), ptr, address.value.size());

    return address;
}

auto ethernet_address_factory::calculate_base_address(
    const std::shared_ptr<ethernet_ip_address> &ip_address,
    const std::shared_ptr<ethernet_ip_address> &ip_mask)
    -> std::optional<std::shared_ptr<ethernet_ip_address>>
{
    if (ip_address == nullptr)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided address pointer is nullptr.");
        return std::nullopt;
    }

    if (ip_mask == nullptr)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided mask pointer is nullptr.");
        return std::nullopt;
    }

    if (const auto *ipv4 = dynamic_cast<ethernet_ipv4_address *>(ip_address.get()))
    {
        const auto *ipv4_mask = dynamic_cast<ethernet_ipv4_address *>(ip_mask.get());
        if (ipv4_mask == nullptr)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided mask is not an IPv4 address.");
            return std::nullopt;
        }

        return ethernet_address_factory::from_uint32_t(ipv4->to_uint32() & ipv4_mask->to_uint32());
    }
    else if (const auto *ipv6 = dynamic_cast<ethernet_ipv6_address *>(ip_address.get()))
    {
        const auto *ipv6_mask = dynamic_cast<ethernet_ipv6_address *>(ip_mask.get());
        if (ipv6_mask == nullptr)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided mask is not an IPv6 address.");
            return std::nullopt;
        }

        auto ip = std::make_shared<ethernet_ipv6_address>();

        for (size_t i = 0; i < ip->value.size(); ++i)
        {
            ip->value[i] = ipv6->value[i] & ipv6_mask->value[i];
        }

        return ip;
    }

    SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided IP address is not IPv4 or IPv6.");

    return std::nullopt;
}

auto ethernet_address_factory::calculate_new_address(
    const std::shared_ptr<ethernet_ip_address> &base_address, const uint64_t host_index)
    -> std::optional<std::shared_ptr<ethernet_ip_address>>
{
    if (base_address == nullptr)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided base address pointer is nullptr.");
        return std::nullopt;
    }

    if (const auto *ipv4 = dynamic_cast<ethernet_ipv4_address *>(base_address.get()))
    {
        const uint64_t sum =
            static_cast<uint64_t>(ipv4->to_uint32()) + static_cast<uint64_t>(host_index);
        if (sum > 0xFFFFFFFFu)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                "Host index {} causes overflow for IPv4 base address {}.", host_index,
                ipv4->to_string());
            return std::nullopt;
        }

        return ethernet_address_factory::from_uint32_t(static_cast<uint32_t>(sum));
    }
    else if (const auto *ipv6 = dynamic_cast<ethernet_ipv6_address *>(base_address.get()))
    {
        auto new_ip = std::make_shared<ethernet_ipv6_address>();

        new_ip->value = ipv6->value;

        uint64_t modified_host_index = host_index;
        for (int i = 7; i >= 0 && modified_host_index != 0; --i)
        {
            uint64_t sum = static_cast<uint64_t>(ipv6->value[i]) + (modified_host_index & 0xFFFF);
            new_ip->value[i] = static_cast<uint16_t>(sum & 0xFFFF);
            modified_host_index = (modified_host_index >> 16) + (sum >> 16);
        }

        if (modified_host_index != 0)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                "Host index {} causes overflow for IPv6 base address {}.", host_index,
                ipv6->to_string());
            return std::nullopt;
        }

        return new_ip;
    }

    SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided IP address is not IPv4 or IPv6.");

    return std::nullopt;
}

auto ethernet_address_factory::calculate_mask(
    const std::shared_ptr<ethernet_ip_address> &ip_address, const uint32_t mask_prefix)
    -> std::optional<std::shared_ptr<ethernet_ip_address>>
{
    if (ip_address == nullptr)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided IP address pointer is nullptr.");
        return std::nullopt;
    }

    if (dynamic_cast<ethernet_ipv4_address *>(ip_address.get()))
    {
        if (mask_prefix == 0 || mask_prefix > 32)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Invalid mask prefix for IPv4: {}.", mask_prefix);
            return std::nullopt;
        }

        const uint32_t ipv4_mask = ~static_cast<uint32_t>(0) << (32 - mask_prefix);
        auto result = ethernet_address_factory::from_uint32_t(ipv4_mask);
        if (!result)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to create IPv4 mask from uint32_t.");
            return std::nullopt;
        }

        return result;
    }
    else if (dynamic_cast<ethernet_ipv6_address *>(ip_address.get()))
    {
        if (mask_prefix == 0 || mask_prefix > 128)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Invalid mask prefix for IPv6: {}.", mask_prefix);
            return std::nullopt;
        }

        uint32_t prefix_length = mask_prefix;
        std::array<uint8_t, 16> mask_bytes = {0};

        for (int i = 0; i < 16; ++i)
        {
            if (prefix_length >= 8)
            {
                mask_bytes[i] = 0xFF;
                prefix_length -= 8;
            }
            else if (prefix_length > 0)
            {
                mask_bytes[i] = static_cast<uint8_t>(0xFF << (8 - prefix_length));
                prefix_length = 0;
            }
            else
            {
                mask_bytes[i] = 0x00;
            }
        }

        auto ip = std::make_shared<ethernet_ipv6_address>();

        for (size_t i = 0; i < ip->value.size(); ++i)
        {
            ip->value[i] = (static_cast<uint16_t>(mask_bytes[2 * i]) << 8) | mask_bytes[2 * i + 1];
        }

        return ip;
    }

    SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided IP address is not IPv4 or IPv6.");

    return std::nullopt;
}

auto ethernet_address_factory::calculate_mask_prefix(
    const std::shared_ptr<ethernet_ip_address> &mask) -> std::optional<uint32_t>
{
    if (mask == nullptr)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided mask pointer is nullptr.");
        return std::nullopt;
    }

    if (const auto *ipv4_mask = dynamic_cast<ethernet_ipv4_address *>(mask.get()))
    {
        const uint32_t mask_value = ipv4_mask->to_uint32();
        uint32_t mask_prefix = 0;
        bool found_zero = false;

        for (int i = 31; i >= 0; --i)
        {
            if ((mask_value >> i) & 0x01)
            {
                if (found_zero)
                {
                    SPDLOG_LOGGER_ERROR(
                        KOMMPOT_LOGGER, "Invalid mask: non-contiguous bits detected.");
                    return std::nullopt;
                }

                ++mask_prefix;
            }
            else
            {
                found_zero = true;
            }
        }

        return mask_prefix;
    }
    else if (const auto *ipv6_mask = dynamic_cast<ethernet_ipv6_address *>(mask.get()))
    {
        uint32_t mask_prefix = 0;
        bool found_zero = false;

        for (size_t i = 0; i < ipv6_mask->value.size(); ++i)
        {
            for (int bit = 15; bit >= 0; --bit)
            {
                if ((ipv6_mask->value[i] >> bit) & 0x01)
                {
                    if (found_zero)
                    {
                        SPDLOG_LOGGER_ERROR(
                            KOMMPOT_LOGGER, "Invalid IPv6 mask: non-contiguous bits detected.");
                        return std::nullopt;
                    }
                    ++mask_prefix;
                }
                else
                {
                    found_zero = true;
                }
            }
        }

        return mask_prefix;
    }

    SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided IP address is not IPv4 or IPv6.");

    return std::nullopt;
}

auto ethernet_address_factory::calculate_address_count(
    const std::shared_ptr<ethernet_ip_address> &ip_address, const uint32_t mask_prefix)
    -> std::optional<uint64_t>
{
    if (ip_address == nullptr)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided IP address pointer is nullptr.");
        return std::nullopt;
    }

    if (dynamic_cast<ethernet_ipv4_address *>(ip_address.get()))
    {
        if (mask_prefix == 0 || mask_prefix > 32)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Invalid mask prefix for IPv4: {}.", mask_prefix);
            return std::nullopt;
        }

        return 1ULL << (32 - mask_prefix);
    }
    else if (dynamic_cast<ethernet_ipv6_address *>(ip_address.get()))
    {
        if (mask_prefix == 0 || mask_prefix > 128)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Invalid mask prefix for IPv6: {}.", mask_prefix);
            return std::nullopt;
        }

        const uint32_t shift = 128 - mask_prefix;
        if (shift >= 64)
        {
            // SATURATED: true count is 2^shift which exceeds uint64_t.
            return std::numeric_limits<uint64_t>::max();
        }
        else
        {
            return 1ULL << shift;
        }
    }

    SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided IP address is not IPv4 or IPv6.");

    return std::nullopt;
}
