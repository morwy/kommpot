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
    if (data == nullptr)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
            "Null pointer provided for data buffer, cannot perform read operation.");
        return false;
    }

    if (size_bytes == 0)
    {
        SPDLOG_LOGGER_ERROR(
            KOMMPOT_LOGGER, "Buffer size provided is zero, cannot perform read operation.");
        return false;
    }

    if (m_connection == nullptr)
    {
        SPDLOG_LOGGER_ERROR(
            KOMMPOT_LOGGER, "Connection is not established, cannot perform read operation.");
        return false;
    }

    auto result = std::visit(
        [&](const auto &s) {
            if constexpr (std::is_same_v<std::decay_t<decltype(s)>,
                              kommpot::http_transfer_configuration>)
            {
                if (s.resource_path.empty())
                {
                    SPDLOG_LOGGER_ERROR(
                        KOMMPOT_LOGGER, "Resource path is empty, cannot perform HTTP transfer.");
                    return false;
                }

                switch (s.type)
                {
                case kommpot::http_transfer_type::GET: {
                    const auto result = m_connection->Get(s.resource_path);
                    if (result->status < 200 || result->status >= 300)
                    {
                        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                            "HTTP GET request failed with status code {} [{}]", result->status,
                            result->status);
                        return false;
                    }

                    std::memcpy(
                        data, result->body.data(), std::min(size_bytes, result->body.size()));

                    return true;
                }
                default:
                    return false;
                }
            }

            return false;
        },
        configuration);

    return result;
}

auto communication_http::write(
    const kommpot::transfer_configuration &configuration, void *data, size_t size_bytes) -> bool
{
    if (data == nullptr)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
            "Null pointer provided for data buffer, cannot perform write operation.");
        return false;
    }

    if (size_bytes == 0)
    {
        SPDLOG_LOGGER_ERROR(
            KOMMPOT_LOGGER, "Buffer size provided is zero, cannot perform write operation.");
        return false;
    }

    if (m_connection == nullptr)
    {
        SPDLOG_LOGGER_ERROR(
            KOMMPOT_LOGGER, "Connection is not established, cannot perform write operation.");
        return false;
    }

    const auto data_str = std::string(static_cast<char *>(data), size_bytes);

    auto result = std::visit(
        [&](const auto &s) {
            if constexpr (std::is_same_v<std::decay_t<decltype(s)>,
                              kommpot::http_transfer_configuration>)
            {
                if (s.resource_path.empty())
                {
                    SPDLOG_LOGGER_ERROR(
                        KOMMPOT_LOGGER, "Resource path is empty, cannot perform HTTP transfer.");
                    return false;
                }

                switch (s.type)
                {
                case kommpot::http_transfer_type::PATCH: {
                    auto result =
                        m_connection->Patch(s.resource_path, data_str, std::string(), nullptr);
                    if (result->status < 200 || result->status >= 300)
                    {
                        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                            "HTTP PATCH request failed with status code {} [{}]", result->status,
                            result->status);
                        return false;
                    }

                    return true;
                }
                case kommpot::http_transfer_type::POST: {
                    auto result =
                        m_connection->Post(s.resource_path, data_str, std::string(), nullptr);
                    if (result->status < 200 || result->status >= 300)
                    {
                        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                            "HTTP POST request failed with status code {} [{}]", result->status,
                            result->status);
                        return false;
                    }

                    return true;
                }
                case kommpot::http_transfer_type::PUT: {
                    auto result =
                        m_connection->Put(s.resource_path, data_str, std::string(), nullptr);
                    if (result->status < 200 || result->status >= 300)
                    {
                        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                            "HTTP PUT request failed with status code {} [{}]", result->status,
                            result->status);
                        return false;
                    }

                    return true;
                }
                case kommpot::http_transfer_type::DELETE_E: {
                    auto result =
                        m_connection->Delete(s.resource_path, data_str, std::string(), nullptr);
                    if (result->status < 200 || result->status >= 300)
                    {
                        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                            "HTTP DELETE request failed with status code {} [{}]", result->status,
                            result->status);
                        return false;
                    }

                    return true;
                }
                default:
                    return false;
                }
            }

            return false;
        },
        configuration);

    return result;
}

auto communication_http::get_error_string(const uint32_t &native_error_code) const -> std::string
{
    return "";
}

auto communication_http::native_handle() const -> void *
{
    return nullptr;
}
