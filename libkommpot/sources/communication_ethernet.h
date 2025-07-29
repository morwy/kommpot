#ifndef COMMUNICATION_ETHERNET_H
#define COMMUNICATION_ETHERNET_H

#pragma once

#include "libkommpot.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

struct ethernet_ipv4_address
{
    std::array<uint8_t, 4> value;

    ethernet_ipv4_address()
    {
        value.fill(0x00);
    }

    ethernet_ipv4_address(const uint32_t &input)
    {
        value.fill(0x00);

        value.at(0) = static_cast<uint8_t>((input >> 24) & 0xFF);
        value.at(1) = static_cast<uint8_t>((input >> 16) & 0xFF);
        value.at(2) = static_cast<uint8_t>((input >> 8) & 0xFF);
        value.at(3) = static_cast<uint8_t>(input & 0xFF);
    }

    auto to_uint32() const -> const uint32_t
    {
        return (value[0] << 24) | (value[1] << 16) | (value[2] << 8) | value[3];
    }

    auto to_string() const -> const std::string
    {
        std::stringstream result;

        for (size_t i = 0; i < value.size(); ++i)
        {
            result << (i > 0 ? "." : "") << std::to_string(value[i]);
        }

        return result.str();
    }
};

struct ethernet_mac_address
{
    std::array<uint8_t, 6> value;

    ethernet_mac_address()
    {
        value.fill(0x00);
    }

    ethernet_mac_address(const uint8_t data[8])
    {
        std::memcpy(value.data(), data, value.size());
    }

    auto to_string() const -> const std::string
    {
        std::stringstream result;

        for (size_t i = 0; i < value.size(); ++i)
        {
            result << (i > 0 ? ":" : "") << std::hex << std::uppercase << std::to_string(value[i]);
        }

        return result.str();
    }
};

struct ethernet_interface_information
{
    std::wstring interface_name = L"";
    std::string adapter_name = "";
    std::wstring description = L"";
    std::wstring dns_suffix = L"";

    ethernet_ipv4_address ipv4_base_address;
    ethernet_ipv4_address ipv4_address;
    ethernet_ipv4_address ipv4_gateway;
    ethernet_ipv4_address ipv4_mask;
    uint32_t ipv4_mask_prefix = 0;
    uint32_t ipv4_max_hosts = 0;

    ethernet_mac_address mac_address;
};

class communication_ethernet : public kommpot::device_communication
{
public:
    explicit communication_ethernet(const kommpot::communication_information &information);
    ~communication_ethernet() override;

    static auto devices(const std::vector<kommpot::device_identification> &identifications)
        -> std::vector<std::shared_ptr<kommpot::device_communication>>;

    auto open() -> bool override;
    auto is_open() -> bool override;
    auto close() -> void override;

    auto endpoints() -> std::vector<kommpot::endpoint_information> override;

    auto read(const kommpot::transfer_configuration &configuration, void *data, size_t size_bytes)
        -> bool override;
    auto write(const kommpot::transfer_configuration &configuration, void *data, size_t size_bytes)
        -> bool override;

    [[nodiscard]] auto get_error_string(const uint32_t &native_error_code) const
        -> std::string override;

    [[nodiscard]] auto native_handle() const -> void * override;

private:
    static constexpr uint32_t M_MAX_CONCURRENT_SEARCH_THREADS = 256;
    static constexpr uint32_t M_TRANSFER_TIMEOUT_MSEC = 2000;

    [[nodiscard]] static auto get_all_interfaces()
        -> const std::vector<ethernet_interface_information>;

    static auto is_alive(const ethernet_ipv4_address &ip, int port) -> bool;
    static auto scan_network(const ethernet_interface_information &interface,
        const kommpot::ethernet_device_identification &identification)
        -> const std::vector<std::shared_ptr<kommpot::device_communication>>;

    [[nodiscard]] static auto set_socket_timeout(int socket, const uint32_t &timeout_msecs) -> const
        bool;
};

#endif // COMMUNICATION_ETHERNET_H
