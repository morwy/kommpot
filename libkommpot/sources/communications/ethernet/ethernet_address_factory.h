#ifndef ETHERNET_ADDRESS_FACTORY_H
#define ETHERNET_ADDRESS_FACTORY_H

#pragma once

#include <communications/ethernet/ethernet_address.h>

#include <memory>
#include <optional>

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
    [[nodiscard]] static auto from_string(const std::string &value)
        -> std::optional<std::shared_ptr<ethernet_ip_address>>;
    [[nodiscard]] static auto from_uint32_t(const uint32_t value)
        -> std::optional<std::shared_ptr<ethernet_ip_address>>;
    [[nodiscard]] static auto from_sockaddr_in(const sockaddr *structure)
        -> std::optional<std::shared_ptr<ethernet_ip_address>>;

    [[nodiscard]] static auto from_array(const uint8_t *ptr, const size_t length)
        -> std::optional<ethernet_mac_address>;

    [[nodiscard]] static auto calculate_base_address(
        const std::shared_ptr<ethernet_ip_address> &ip_address,
        const std::shared_ptr<ethernet_ip_address> &mask)
        -> std::optional<std::shared_ptr<ethernet_ip_address>>;

    [[nodiscard]] static auto calculate_new_address(
        const std::shared_ptr<ethernet_ip_address> &base_address, const uint64_t host_index)
        -> std::optional<std::shared_ptr<ethernet_ip_address>>;

    [[nodiscard]] static auto calculate_mask(const std::shared_ptr<ethernet_ip_address> &ip_address,
        const uint32_t mask_prefix) -> std::optional<std::shared_ptr<ethernet_ip_address>>;

    [[nodiscard]] static auto calculate_mask_prefix(
        const std::shared_ptr<ethernet_ip_address> &mask) -> std::optional<uint32_t>;

    [[nodiscard]] static auto calculate_address_count(
        const std::shared_ptr<ethernet_ip_address> &ip_address, const uint32_t mask_prefix)
        -> std::optional<uint64_t>;
};

#endif // ETHERNET_ADDRESS_FACTORY_H
