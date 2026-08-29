#include "communication_http.h"

#include <communications/ethernet/communication_ethernet.h>
#include <kommpot_core.h>
#include <libkommpot.h>
#include <third-party/spdlog/include/spdlog/spdlog.h>

#include <algorithm>
#include <cstring>

namespace {
    auto transfer_type_to_string(const kommpot::http_transfer_type &type) -> std::string
    {
        switch (type)
        {
        case kommpot::http_transfer_type::GET: {
            return "GET";
        }
        case kommpot::http_transfer_type::POST: {
            return "POST";
        }
        case kommpot::http_transfer_type::PUT: {
            return "PUT";
        }
        case kommpot::http_transfer_type::PATCH: {
            return "PATCH";
        }
        case kommpot::http_transfer_type::DELETE_E: {
            return "DELETE";
        }
        default: {
            return "";
        }
        }
    }
} // namespace

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
    if (m_handle != nullptr)
    {
        return true;
    }

    const auto *configuration =
        std::get_if<kommpot::http_device_configuration>(&m_configuration_variant);
    if (configuration != nullptr)
    {
        m_configuration = *configuration;
    }

    m_handle = curl_easy_init();
    if (m_handle == nullptr)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "curl_easy_init() failed creating the HTTP session!");
        return false;
    }

    m_response.clear();
    m_response_offset = 0;

    /**
     * An empty cookie file enables the in-memory cookie engine, so that a session established by
     * one request stays valid for the following ones performed on the same communication.
     */
    curl_easy_setopt(m_handle, CURLOPT_COOKIEFILE, "");

    curl_easy_setopt(
        m_handle, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(m_configuration.timeout_ms));
    curl_easy_setopt(m_handle, CURLOPT_TIMEOUT_MS, static_cast<long>(m_configuration.timeout_ms));
    curl_easy_setopt(m_handle, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(m_handle, CURLOPT_FOLLOWLOCATION, m_configuration.follow_redirects ? 1L : 0L);
    curl_easy_setopt(m_handle, CURLOPT_SSL_VERIFYPEER, m_configuration.verify_peer ? 1L : 0L);
    curl_easy_setopt(m_handle, CURLOPT_SSL_VERIFYHOST, m_configuration.verify_peer ? 2L : 0L);

    if (!m_configuration.user_agent.empty())
    {
        curl_easy_setopt(m_handle, CURLOPT_USERAGENT, m_configuration.user_agent.c_str());
    }

    curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, &m_response);

    return true;
}

auto communication_http::is_open() -> bool
{
    return m_handle != nullptr;
}

auto communication_http::close() -> void
{
    if (m_handle == nullptr)
    {
        return;
    }

    curl_easy_cleanup(m_handle);
    m_handle = nullptr;

    m_response.clear();
    m_response_offset = 0;
}

auto communication_http::endpoints() -> std::vector<kommpot::endpoint_information>
{
    auto information = kommpot::endpoint_information();

    information.address = m_identification.port;
    information.type = kommpot::endpoint_type::DUPLEX;

    return {information};
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

    const auto *http_configuration =
        std::get_if<kommpot::http_transfer_configuration>(&configuration);
    if (http_configuration == nullptr)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided transfer configuration is not HTTP.");
        return false;
    }

    http_configuration->bytes_read = 0;

    if (m_response_offset >= m_response.size())
    {
        return false;
    }

    const size_t length = std::min(size_bytes, m_response.size() - m_response_offset);
    std::memcpy(data, m_response.data() + m_response_offset, length);
    m_response_offset += length;
    http_configuration->bytes_read = length;

    return true;
}

auto communication_http::write(
    const kommpot::transfer_configuration &configuration, void *data, size_t size_bytes) -> bool
{
    const auto *http_configuration =
        std::get_if<kommpot::http_transfer_configuration>(&configuration);
    if (http_configuration == nullptr)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided transfer configuration is not HTTP.");
        return false;
    }

    return perform(*http_configuration, data, size_bytes);
}

auto communication_http::get_error_string(const uint32_t &native_error_code) const -> std::string
{
    return curl_easy_strerror(static_cast<CURLcode>(native_error_code));
}

auto communication_http::native_handle() const -> void *
{
    return m_handle;
}

auto communication_http::write_callback(char *data, size_t size, size_t count, void *user_data)
    -> size_t
{
    const size_t length = size * count;
    auto *buffer = static_cast<std::string *>(user_data);
    buffer->append(data, length);
    return length;
}

auto communication_http::build_url(const std::string &resource_path) const -> std::string
{
    const uint16_t default_port = m_identification.use_tls ? 443 : 80;

    std::string url = m_identification.use_tls ? "https://" : "http://";
    url += m_identification.address;

    if (m_identification.port != 0 && m_identification.port != default_port)
    {
        url += ":" + std::to_string(m_identification.port);
    }

    if (resource_path.front() != '/')
    {
        url += "/";
    }

    url += resource_path;

    return url;
}

auto communication_http::perform(const kommpot::http_transfer_configuration &configuration,
    void *data, size_t size_bytes) -> bool
{
    if (m_handle == nullptr)
    {
        SPDLOG_LOGGER_ERROR(
            KOMMPOT_LOGGER, "Connection is not established, cannot perform the HTTP transfer.");
        return false;
    }

    if (configuration.resource_path.empty())
    {
        SPDLOG_LOGGER_ERROR(
            KOMMPOT_LOGGER, "Resource path is empty, cannot perform the HTTP transfer.");
        return false;
    }

    const std::string method = transfer_type_to_string(configuration.type);
    if (method.empty())
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Provided HTTP transfer type is not supported.");
        return false;
    }

    m_response.clear();
    m_response_offset = 0;

    const char *body = configuration.body.c_str();
    size_t body_size = configuration.body.size();
    if (configuration.body.empty() && data != nullptr && size_bytes > 0)
    {
        body = static_cast<const char *>(data);
        body_size = size_bytes;
    }

    const std::string url = build_url(configuration.resource_path);

    curl_easy_setopt(m_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_handle, CURLOPT_CUSTOMREQUEST, nullptr);
    curl_easy_setopt(m_handle, CURLOPT_HTTPGET, 1L);

    if (configuration.type != kommpot::http_transfer_type::GET)
    {
        curl_easy_setopt(m_handle, CURLOPT_POSTFIELDS, body);
        curl_easy_setopt(m_handle, CURLOPT_POSTFIELDSIZE, static_cast<long>(body_size));

        if (configuration.type == kommpot::http_transfer_type::POST)
        {
            curl_easy_setopt(m_handle, CURLOPT_POST, 1L);
        }
        else
        {
            curl_easy_setopt(m_handle, CURLOPT_CUSTOMREQUEST, method.c_str());
        }
    }

    curl_slist *headers = nullptr;
    for (const auto &header : configuration.headers)
    {
        headers = curl_slist_append(headers, (header.first + ": " + header.second).c_str());
    }

    if (!configuration.content_type.empty())
    {
        headers =
            curl_slist_append(headers, ("Content-Type: " + configuration.content_type).c_str());
    }

    curl_easy_setopt(m_handle, CURLOPT_HTTPHEADER, headers);

    const auto result = curl_easy_perform(m_handle);

    curl_easy_setopt(m_handle, CURLOPT_HTTPHEADER, nullptr);
    curl_slist_free_all(headers);

    if (result != CURLE_OK)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "HTTP {} request to {} failed with error {} [{}]",
            method, url, curl_easy_strerror(result), static_cast<int>(result));
        return false;
    }

    long status_code = 0;
    curl_easy_getinfo(m_handle, CURLINFO_RESPONSE_CODE, &status_code);
    if (status_code >= M_MINIMAL_ERROR_STATUS_CODE)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "HTTP {} request to {} failed with status code {}",
            method, url, status_code);
        return false;
    }

    return true;
}
