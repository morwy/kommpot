#ifndef ETHERNET_SOCKET_H
#define ETHERNET_SOCKET_H

#pragma once

#include <array>
#include <cstring>
#include <iomanip>
#include <sstream>

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

    ethernet_ipv4_address(const std::string &str)
    {
        size_t idx = 0;

        std::string token;
        std::istringstream iss(str);

        while (std::getline(iss, token, '.') && idx < value.size())
        {
            try
            {
                int num = std::stoi(token);
                if (num < 0 || num > 255)
                {
                    throw std::out_of_range("IPv4 segment out of range");
                }

                value.at(idx++) = static_cast<uint8_t>(num);
            }
            catch (...)
            {
                value.fill(0x00);
                break;
            }
        }
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

enum class ethernet_protocol_type
{
    UNKNOWN = 0,
    TCP = 1,
    UDP = 2,
};

class ethernet_socket
{
public:
    explicit ethernet_socket(const ethernet_ipv4_address &ip_address, const uint16_t &port,
        const ethernet_protocol_type &protocol);
    ~ethernet_socket();

    [[nodiscard]] auto connect() -> const bool;
    [[nodiscard]] auto disconnect() -> const bool;

    [[nodiscard]] auto is_connected() const -> const bool;

    [[nodiscard]] auto read(void *data, size_t size_bytes) const -> bool;
    [[nodiscard]] auto write(void *data, size_t size_bytes) const -> bool;

    [[nodiscard]] auto set_timeout(const uint32_t &timeout_msecs) -> const bool;

    [[nodiscard]] auto hostname() const -> const std::string;
    [[nodiscard]] auto mac_address() const -> const ethernet_mac_address;

    [[nodiscard]] auto to_string() const -> const std::string;

private:
    const ethernet_protocol_type m_protocol;
    const ethernet_ipv4_address m_ip_address;
    const uint16_t m_port;

    /**
     * @attention socket() function returns different errors on Windows and other OSes.
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

    /**
     * @attention unifying error code types under standard int32_t.
     */
    [[nodiscard]] static auto get_last_error_code_as_string() -> const std::string;
    [[nodiscard]] static auto get_error_code() -> const int32_t;
    [[nodiscard]] static auto get_error_code_as_string(const int32_t &error_code)
        -> const std::string;
};

#endif // ETHERNET_SOCKET_H
