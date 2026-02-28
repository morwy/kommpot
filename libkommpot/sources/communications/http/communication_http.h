#ifndef COMMUNICATION_HTTP_H
#define COMMUNICATION_HTTP_H

#pragma once

#include <third-party/cpp-httplib/httplib.h>

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

    auto read(const kommpot::transfer_configuration &configuration, void *data, size_t size_bytes)
        -> bool override;
    auto write(const kommpot::transfer_configuration &configuration, void *data, size_t size_bytes)
        -> bool override;

    [[nodiscard]] auto get_error_string(const uint32_t &native_error_code) const
        -> std::string override;

    [[nodiscard]] auto native_handle() const -> void * override;

private:
    kommpot::http_device_identification m_identification;
    std::unique_ptr<httplib::Client> m_connection = nullptr;
};

#endif // COMMUNICATION_HTTP_H
