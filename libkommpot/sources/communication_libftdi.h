#ifndef COMMUNICATION_LIBFTDI_H
#define COMMUNICATION_LIBFTDI_H

#pragma once

#include "libkommpot.h"

#include "third-party/libftdi/src/ftdi.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class communication_libftdi : public kommpot::device_communication
{
public:
    static constexpr uint32_t VENDOR_ID = 0x0403;

    explicit communication_libftdi(const kommpot::communication_information &information);
    ~communication_libftdi() override;

    static auto devices(const std::vector<kommpot::device_identification> &identifications)
        -> std::vector<std::unique_ptr<kommpot::device_communication>>;

    auto open() -> bool override;
    auto is_open() -> bool override;
    void close() override;

    auto endpoints() -> std::vector<kommpot::endpoint_information> override;

    auto read(const kommpot::transfer_configuration &configuration, void *data, size_t size_bytes)
        -> bool override;
    auto write(const kommpot::transfer_configuration &configuration, void *data, size_t size_bytes)
        -> bool override;

    [[nodiscard]] auto get_error_string(const uint32_t &native_error_code) const
        -> std::string override;

    [[nodiscard]] auto native_handle() const -> void * override;

private:
    static ftdi_context *m_ftdi_context;
    bool m_is_device_opened = false;
    static constexpr uint32_t M_TRANSFER_TIMEOUT_MSEC = 2000;

    static auto get_port_path(libusb_device *device) -> std::string;
};

#endif // COMMUNICATION_LIBFTDI_H
