#ifndef COMMUNICATION_HTTP_H
#define COMMUNICATION_HTTP_H

#pragma once

#include <curl/curl.h>

#include <libkommpot.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class communication_http : public kommpot::device_communication
{
public:
    explicit communication_http(const kommpot::http_device_identification &identification);
    ~communication_http() override;

    static auto devices(const std::vector<kommpot::device_identification> &identifications)
        -> std::vector<std::shared_ptr<kommpot::device_communication>>;

    auto open() -> bool override;
    auto is_open() -> bool override;
    auto close() -> void override;

    auto endpoints() -> std::vector<kommpot::endpoint_information> override;

    /**
     * @brief drains the response body buffered by the preceding write() call.
     * @return true while bytes were copied, false once the buffered body is exhausted.
     * @attention no request is performed here, use write() to perform one.
     */
    auto read(const kommpot::transfer_configuration &configuration, void *data, size_t size_bytes)
        -> bool override;

    /**
     * @brief performs the request described by the configuration, including GET.
     * @attention the response body is buffered and has to be retrieved by read().
     */
    auto write(const kommpot::transfer_configuration &configuration, void *data, size_t size_bytes)
        -> bool override;

    [[nodiscard]] auto get_error_string(const uint32_t &native_error_code) const
        -> std::string override;

    [[nodiscard]] auto native_handle() const -> void * override;

private:
    kommpot::http_device_identification m_identification;
    kommpot::http_device_configuration m_configuration;

    CURL *m_handle = nullptr;

    /**
     * @brief holds the body of the last performed request together with the drain position.
     */
    std::string m_response = "";
    size_t m_response_offset = 0;

    static constexpr long M_MINIMAL_ERROR_STATUS_CODE = 400;

    static auto write_callback(char *data, size_t size, size_t count, void *user_data) -> size_t;

    [[nodiscard]] auto build_url(const std::string &resource_path) const -> std::string;

    auto perform(const kommpot::http_transfer_configuration &configuration, void *data,
        size_t size_bytes) -> bool;
};

#endif // COMMUNICATION_HTTP_H
