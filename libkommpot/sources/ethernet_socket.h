#ifndef ETHERNET_SOCKET_H
#define ETHERNET_SOCKET_H

#pragma once

#include <ethernet_address.h>
#include <libkommpot.h>

#include <cstring>

class ethernet_socket
{
public:
    ethernet_socket();
    ~ethernet_socket();

    [[nodiscard]] auto initialize(const std::shared_ptr<ethernet_ip_address> ip_address,
        const uint16_t &port, const kommpot::ethernet_protocol_type &protocol) -> const bool;

    [[nodiscard]] auto connect() -> const bool;
    [[nodiscard]] auto disconnect() -> const bool;

    [[nodiscard]] auto is_connected() const -> const bool;

    [[nodiscard]] auto read(void *data, size_t size_bytes) const -> const bool;
    [[nodiscard]] auto write(void *data, size_t size_bytes) const -> const bool;

    [[nodiscard]] auto set_timeout(const uint32_t &timeout_msecs) -> const bool;

    [[nodiscard]] auto hostname() const -> const std::string;
    [[nodiscard]] auto mac_address() const -> const ethernet_mac_address;

    [[nodiscard]] auto native_handle() const -> void *;

    [[nodiscard]] auto to_string() const -> const std::string;

private:
    kommpot::ethernet_protocol_type m_protocol = kommpot::ethernet_protocol_type::UNKNOWN;
    std::shared_ptr<ethernet_ip_address> m_ip_address = nullptr;
    int32_t m_ip_family = 0;
    uint16_t m_port = 0;

    /**
     * @attention socket() function returns different error on Windows and other OSes.
     */
#ifdef _WIN32
    static constexpr uint64_t ETH_INVALID_SOCKET = ~0;
#else
    static constexpr uint64_t ETH_INVALID_SOCKET = -1;
#endif

    /**
     * @attention socket error value is the same on Windows and other OSes.
     */
    static constexpr int32_t ETH_SOCKET_ERROR = -1;

    uint64_t m_handle = ETH_INVALID_SOCKET;
    std::string m_hostname = "";
    ethernet_mac_address m_mac_address;
    bool m_is_connected = false;

    auto close_socket() -> const bool;

    [[nodiscard]] auto read_out_hostname(std::string &hostname) -> const bool;
    [[nodiscard]] auto read_out_mac_address(ethernet_mac_address &mac_address) -> const bool;
};

#endif // ETHERNET_SOCKET_H
