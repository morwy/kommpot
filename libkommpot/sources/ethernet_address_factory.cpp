#include <ethernet_address_factory.h>

#include <kommpot_core.h>

auto ethernet_address_factory::from_string(
    const std::string &value, std::shared_ptr<ethernet_ip_address> &address) -> bool
{
    if (value.find(".") != std::string::npos)
    {
        size_t index = 0;

        std::string token = "";
        std::istringstream iss(value);

        auto ip = std::make_shared<ethernet_ipv4_address>();

        while (std::getline(iss, token, '.') && index < value.size())
        {
            const auto digit = std::stoi(token);
            if (digit < 0 || digit > 255)
            {
                SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Invalid IPv4 address: {}.", value);
                return false;
            }

            ip->value.at(index++) = static_cast<uint8_t>(digit);
        }

        address = ip;
    }
    else if (value.find(":") != std::string::npos)
    {
        auto ip = std::make_shared<ethernet_ipv6_address>();

        uint8_t bytes[16] = {0};
        if (inet_pton(AF_INET6, value.c_str(), bytes) != 1)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Invalid IPv6 address: {}.", value);
            return false;
        }

        for (int i = 0; i < 6; ++i)
        {
            ip->value[i] = (static_cast<uint16_t>(bytes[2 * i]) << 8) | bytes[2 * i + 1];
        }

        address = ip;
    }
    else
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided IP address is not IPv4 or IPv6.");
        return false;
    }

    return true;
}

auto ethernet_address_factory::from_uint32_t(
    const uint32_t &value, std::shared_ptr<ethernet_ip_address> &address) -> bool
{
    auto ip = std::make_shared<ethernet_ipv4_address>();

    ip->value.at(0) = static_cast<uint8_t>((value >> 24) & 0xFF);
    ip->value.at(1) = static_cast<uint8_t>((value >> 16) & 0xFF);
    ip->value.at(2) = static_cast<uint8_t>((value >> 8) & 0xFF);
    ip->value.at(3) = static_cast<uint8_t>(value & 0xFF);

    address = ip;

    return true;
}

auto ethernet_address_factory::from_sockaddr_in(
    const sockaddr *structure, std::shared_ptr<ethernet_ip_address> &address) -> bool
{
    if (structure->sa_family == AF_INET)
    {
        auto ip = std::make_shared<ethernet_ipv4_address>();

        const sockaddr_in *ipv4 = reinterpret_cast<const sockaddr_in *>(structure);
        std::memcpy(ip->value.data(), &ipv4->sin_addr, ip->value.size());

        address = ip;
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

        address = ip;
    }
    else
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided IP address is not IPv4 or IPv6.");
        return false;
    }

    return true;
}

auto ethernet_address_factory::calculate_base_address(
    const std::shared_ptr<ethernet_ip_address> ip_address,
    const std::shared_ptr<ethernet_ip_address> ip_mask,
    std::shared_ptr<ethernet_ip_address> &base_address) -> bool
{
    if (const auto *ipv4 = dynamic_cast<ethernet_ipv4_address *>(ip_address.get()))
    {
        const auto *ipv4_mask = dynamic_cast<ethernet_ipv4_address *>(ip_mask.get());
        if (ipv4_mask == nullptr)
        {
            return false;
        }

        if (!ethernet_address_factory::from_uint32_t(
                ipv4->to_uint32() & ipv4_mask->to_uint32(), base_address))
        {
            return false;
        }

        return true;
    }
    else if (const auto *ipv6 = dynamic_cast<ethernet_ipv6_address *>(ip_address.get()))
    {
        const auto *ipv6_mask = dynamic_cast<ethernet_ipv6_address *>(ip_mask.get());
        if (ipv6_mask == nullptr)
        {
            return false;
        }

        std::array<uint16_t, 8> base_address_bytes = {0};

        for (size_t i = 0; i < 8; ++i)
        {
            base_address_bytes[i] = ipv6->value[i] & ipv6_mask->value[i];
        }

        auto ip = std::make_shared<ethernet_ipv6_address>();

        std::memcpy(ip->value.data(), base_address_bytes.data(), base_address_bytes.size());

        base_address = ip;

        return true;
    }
    else
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided IP address is not IPv4 or IPv6.");
        return false;
    }

    return true;
}

auto ethernet_address_factory::calculate_new_address(
    const std::shared_ptr<ethernet_ip_address> base_address, const uint32_t &host_index,
    std::shared_ptr<ethernet_ip_address> &new_address) -> bool
{
    if (const auto *ipv4 = dynamic_cast<ethernet_ipv4_address *>(base_address.get()))
    {
        if (!ethernet_address_factory::from_uint32_t(ipv4->to_uint32() + host_index, new_address))
        {
            return false;
        }

        return true;
    }
    else if (const auto *ipv6 = dynamic_cast<ethernet_ipv6_address *>(base_address.get()))
    {
        auto ip = std::make_shared<ethernet_ipv6_address>();

        uint64_t modified_host_index = host_index;
        for (int i = 7; i >= 0 && modified_host_index != 0; --i)
        {
            uint64_t sum = static_cast<uint64_t>(ipv6->value[i]) + (modified_host_index & 0xFFFF);
            ip->value[i] = static_cast<uint16_t>(sum & 0xFFFF);
            modified_host_index = (modified_host_index >> 16) + (sum >> 16);
        }

        new_address = ip;

        return true;
    }
    else
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided IP address is not IPv4 or IPv6.");
        return false;
    }

    return true;
}

auto ethernet_address_factory::calculate_mask(const std::shared_ptr<ethernet_ip_address> ip_address,
    const uint32_t &mask_prefix, std::shared_ptr<ethernet_ip_address> &mask) -> bool
{
    if (dynamic_cast<ethernet_ipv4_address *>(ip_address.get()))
    {
        const uint32_t ipv4_mask = (mask_prefix == 0) ? 0 : (~0u << (32 - mask_prefix));
        if (!ethernet_address_factory::from_uint32_t(ipv4_mask, mask))
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to create IPv4 mask from uint32_t.");
            return false;
        }

        return true;
    }
    else if (dynamic_cast<ethernet_ipv6_address *>(ip_address.get()))
    {
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

        std::memcpy(ip->value.data(), mask_bytes.data(), mask_bytes.size());

        mask = ip;

        return true;
    }
    else
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided IP address is not IPv4 or IPv6.");
        return false;
    }

    return true;
}

auto ethernet_address_factory::calculate_max_hosts(
    const std::shared_ptr<ethernet_ip_address> ip_address, const uint32_t &mask_prefix,
    uint32_t &max_hosts) -> bool
{
    if (dynamic_cast<ethernet_ipv4_address *>(ip_address.get()))
    {
        max_hosts = 1 << (32 - mask_prefix);
        return true;
    }
    else if (dynamic_cast<ethernet_ipv6_address *>(ip_address.get()))
    {
        max_hosts = 1 << (128 - mask_prefix);
        return true;
    }
    else
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided IP address is not IPv4 or IPv6.");
        return false;
    }

    return true;
}
