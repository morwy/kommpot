#ifndef COMMUNICATION_LIBUSB_H
#define COMMUNICATION_LIBUSB_H

#pragma once

#include "libkommpot.h"

#include "third-party/libusb-cmake/libusb/libusb/libusb.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class communication_libusb : public kommpot::device_communication
{
public:
    explicit communication_libusb(const kommpot::usb_device_identification &identification);
    ~communication_libusb() override;

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
    kommpot::usb_device_identification m_identification;
    static libusb_context *m_libusb_context;
    libusb_device_handle *m_device_handle = nullptr;
    static constexpr uint32_t M_TRANSFER_TIMEOUT_MSEC = 2000;
    static constexpr uint32_t M_MAX_USB_DESCRIPTOR_LENGTH_BYTES = 255;

    static auto read_descriptor(libusb_device_handle *device_handle, uint8_t descriptor_index)
        -> std::string;
    static auto get_port_path(libusb_device_handle *device_handle) -> std::string;

    auto transfer(const kommpot::transfer_configuration &configuration, void *data,
        size_t size_bytes) -> bool;

    static auto strip_trailing_lf(std::string string) -> std::string;
    static auto log(libusb_context *context, libusb_log_level level, const char *message) -> void;
};

#endif // COMMUNICATION_LIBUSB_H
