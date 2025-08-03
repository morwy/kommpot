#ifndef ETHERNET_ADDRESS_FACTORY_H
#define ETHERNET_ADDRESS_FACTORY_H

#pragma once

#include <ethernet_address.h>

#ifdef _WIN32
// clang-format off
#    include <ws2tcpip.h>
// clang-format on
#else
#    include <arpa/inet.h>
#endif

class ethernet_address_factory
{
public:
    [[nodiscard]] static auto from_string(
        const std::string &value, std::shared_ptr<ethernet_ip_address> &address) -> bool;
    [[nodiscard]] static auto from_uint32_t(
        const uint32_t &value, std::shared_ptr<ethernet_ip_address> &address) -> bool;
    [[nodiscard]] static auto from_sockaddr_in(
        const sockaddr *structure, std::shared_ptr<ethernet_ip_address> &address) -> bool;

    [[nodiscard]] static auto calculate_base_address(
        const std::shared_ptr<ethernet_ip_address> ip_address,
        const std::shared_ptr<ethernet_ip_address> mask,
        std::shared_ptr<ethernet_ip_address> &base_address) -> bool;

    [[nodiscard]] static auto calculate_new_address(
        const std::shared_ptr<ethernet_ip_address> base_address, const uint32_t &host_index,
        std::shared_ptr<ethernet_ip_address> &new_address) -> bool;

    [[nodiscard]] static auto calculate_max_hosts(
        const std::shared_ptr<ethernet_ip_address> ip_address, const uint32_t &mask_prefix,
        uint32_t &max_hosts) -> bool;
};

#endif // ETHERNET_ADDRESS_FACTORY_H
