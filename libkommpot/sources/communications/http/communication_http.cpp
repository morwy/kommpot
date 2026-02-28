#include "communication_http.h"

#include <communications/ethernet/communication_ethernet.h>
#include <kommpot_core.h>
#include <libkommpot.h>
#include <third-party/spdlog/include/spdlog/spdlog.h>

communication_http::communication_http(const kommpot::http_device_identification &identification)
    : kommpot::device_communication(identification)
{
    m_type = kommpot::communication_type::HTTP;
    m_identification = identification;
}

communication_http::~communication_http()
{
    close();
}

auto communication_http::devices(const std::vector<kommpot::device_identification> &identifications)
    -> std::vector<std::shared_ptr<kommpot::device_communication>>
{
    return communication_ethernet::devices(identifications);
}

auto communication_http::open() -> bool
{
    if (m_connection != nullptr)
    {
        return is_open();
    }

    if (m_identification.port != 0)
    {
        m_connection =
            std::make_unique<httplib::Client>(m_identification.address, m_identification.port);
    }
    else
    {
        m_connection = std::make_unique<httplib::Client>(m_identification.address);
    }

    return m_connection != nullptr;
}

auto communication_http::is_open() -> bool
{
    if (m_connection == nullptr)
    {
        return false;
    }

    return m_connection->is_valid() && m_connection->is_socket_open();
}

auto communication_http::close() -> void
{
    if (m_connection != nullptr)
    {
        m_connection->stop();
        m_connection.reset();
        m_connection = nullptr;
    }
}

auto communication_http::endpoints() -> std::vector<kommpot::endpoint_information>
{
    if (m_connection == nullptr)
    {
        return {};
    }

    const auto port = m_connection->port();

    return {kommpot::endpoint_information{kommpot::endpoint_type::DUPLEX, port}};
}

auto communication_http::read(
    const kommpot::transfer_configuration &configuration, void *data, size_t size_bytes) -> bool
{
    if (m_connection == nullptr)
    {
        return false;
    }

    auto result = std::visit(
        [&](const auto &s) {
            if constexpr (std::is_same_v<std::decay_t<decltype(s)>,
                              kommpot::bulk_transfer_configuration>)
            {
                return true;
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(s)>,
                                   kommpot::control_transfer_configuration>)
            {
                return true;
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(s)>,
                                   kommpot::interrupt_transfer_configuration>)
            {
                return true;
            }
        },
        configuration);
    return result;
}

auto communication_http::write(
    const kommpot::transfer_configuration &configuration, void *data, size_t size_bytes) -> bool
{
    return false;
}

auto communication_http::get_error_string(const uint32_t &native_error_code) const -> std::string
{
    return "";
}

auto communication_http::native_handle() const -> void *
{
    return nullptr;
}
