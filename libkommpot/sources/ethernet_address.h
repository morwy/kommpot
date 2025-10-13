#ifndef ETHERNET_ADDRESS_H
#define ETHERNET_ADDRESS_H

#pragma once

#include <array>
#include <cstring>
#include <iomanip>
#include <sstream>

class ethernet_ip_address
{
public:
    ethernet_ip_address() = default;
    virtual auto to_string() const -> const std::string = 0;
};

class ethernet_ipv4_address : public ethernet_ip_address
{
public:
    ethernet_ipv4_address()
    {
        value.fill(0x00);
    }

    auto to_uint32() const -> const uint32_t
    {
        return (value[0] << 24) | (value[1] << 16) | (value[2] << 8) | value[3];
    }

    auto to_string() const -> const std::string override
    {
        std::stringstream result;

        for (size_t i = 0; i < value.size(); ++i)
        {
            result << (i > 0 ? "." : "") << std::to_string(value[i]);
        }

        return result.str();
    }

private:
    friend class ethernet_address_factory;
    std::array<uint8_t, 4> value;
};

class ethernet_ipv6_address : public ethernet_ip_address
{
public:
    ethernet_ipv6_address()
    {
        value.fill(0x00);
    }

    auto to_string() const -> const std::string override
    {
        std::stringstream result;

        for (size_t i = 0; i < value.size(); ++i)
        {
            result << (i > 0 ? ":" : "") << std::to_string(value[i]);
        }

        return result.str();
    }

private:
    friend class ethernet_address_factory;
    std::array<uint16_t, 8> value;
};

class ethernet_mac_address
{
public:
    ethernet_mac_address()
    {
        value.fill(0x00);
    }

    ethernet_mac_address(const uint8_t data[8])
    {
        std::memcpy(value.data(), data, value.size());
    }

    auto empty() const -> bool
    {
        return memcmp(value.data(), "\x00\x00\x00\x00\x00\x00", value.size()) == 0;
    }

    auto to_string() const -> const std::string
    {
        std::stringstream result;

        result << std::uppercase << std::hex << std::setfill('0');

        for (size_t i = 0; i < value.size(); ++i)
        {
            result << std::setw(2) << static_cast<int>(value[i]);

            if (i != value.size() - 1)
            {
                result << ":";
            }
        }

        return result.str();
    }

private:
    friend class ethernet_address_factory;
    std::array<uint8_t, 6> value;
};

#endif // ETHERNET_ADDRESS_H
