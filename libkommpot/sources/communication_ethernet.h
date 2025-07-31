#ifndef COMMUNICATION_ETHERNET_H
#define COMMUNICATION_ETHERNET_H

#pragma once

#include <ethernet_socket.h>
#include <libkommpot.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class communication_ethernet : public kommpot::device_communication
{
public:
    explicit communication_ethernet(const kommpot::ethernet_device_identification &identification);
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
    kommpot::ethernet_device_identification m_identification;
    ethernet_socket m_socket;
    static constexpr uint32_t M_MAX_CONCURRENT_SEARCH_THREADS = 256;
    static constexpr uint32_t M_TRANSFER_TIMEOUT_MSEC = 2000;

    [[nodiscard]] static auto get_all_interfaces()
        -> const std::vector<ethernet_interface_information>;

    static auto is_host_reachable(const ethernet_ipv4_address &ip, const uint16_t port,
        kommpot::ethernet_device_identification &information) -> bool;
    static auto scan_network_for_hosts(const ethernet_interface_information &interface,
        const kommpot::ethernet_device_identification &identification)
        -> const std::vector<std::shared_ptr<kommpot::device_communication>>;
};

#endif // COMMUNICATION_ETHERNET_H
