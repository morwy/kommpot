#ifndef ETHERNET_ADDRESS_H
#define ETHERNET_ADDRESS_H

#pragma once

#include <array>
#include <cstdint>
#include <string>

class ethernet_ip_address
{
public:
    constexpr ethernet_ip_address() = default;
    virtual ~ethernet_ip_address() = default;

    virtual auto to_string() const -> std::string = 0;
};

class ethernet_ipv4_address : public ethernet_ip_address
{
public:
    ethernet_ipv4_address();

    auto to_uint32() const -> uint32_t;
    auto to_string() const -> std::string override;

    auto operator==(const ethernet_ipv4_address &other) const noexcept -> bool;
    auto operator!=(const ethernet_ipv4_address &other) const noexcept -> bool;
    auto operator<(const ethernet_ipv4_address &other) const noexcept -> bool;
    auto operator<=(const ethernet_ipv4_address &other) const noexcept -> bool;
    auto operator>(const ethernet_ipv4_address &other) const noexcept -> bool;
    auto operator>=(const ethernet_ipv4_address &other) const noexcept -> bool;

private:
    friend class ethernet_address_factory;
    std::array<uint8_t, 4> value;
};

class ethernet_ipv6_address : public ethernet_ip_address
{
public:
    ethernet_ipv6_address();

    auto to_string() const -> std::string override;

    auto operator==(const ethernet_ipv6_address &other) const noexcept -> bool;
    auto operator!=(const ethernet_ipv6_address &other) const noexcept -> bool;
    auto operator<(const ethernet_ipv6_address &other) const noexcept -> bool;
    auto operator<=(const ethernet_ipv6_address &other) const noexcept -> bool;
    auto operator>(const ethernet_ipv6_address &other) const noexcept -> bool;
    auto operator>=(const ethernet_ipv6_address &other) const noexcept -> bool;

private:
    friend class ethernet_address_factory;
    std::array<uint16_t, 8> value;
};

class ethernet_mac_address
{
public:
    ethernet_mac_address();

    auto empty() const -> bool;
    auto to_string() const -> std::string;

    auto operator==(const ethernet_mac_address &other) const noexcept -> bool;
    auto operator!=(const ethernet_mac_address &other) const noexcept -> bool;
    auto operator<(const ethernet_mac_address &other) const noexcept -> bool;
    auto operator<=(const ethernet_mac_address &other) const noexcept -> bool;
    auto operator>(const ethernet_mac_address &other) const noexcept -> bool;
    auto operator>=(const ethernet_mac_address &other) const noexcept -> bool;

private:
    friend class ethernet_address_factory;
    std::array<uint8_t, 6> value;
};

#endif // ETHERNET_ADDRESS_H
