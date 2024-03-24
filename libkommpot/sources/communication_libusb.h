#ifndef COMMUNICATION_LIBUSB_H
#define COMMUNICATION_LIBUSB_H

#pragma once

#include "libkommpot.h"

#include "third-party/libusb-cmake/libusb/libusb/libusb.h"

class communication_libusb : public kommpot::device_communication
{
public:
    communication_libusb(const kommpot::communication_information &information);

    static auto get_available_devices(const kommpot::device_identification &identification)
        -> std::vector<std::unique_ptr<kommpot::device_communication>>;

    auto open() -> bool override;
    auto is_open() -> bool override;
    void close() override;

    auto read(const int &endpoint_address, void *data, size_t size_bytes) -> bool override;
    auto write(const int &endpoint_address, void *data, size_t size_bytes) -> bool override;

    [[nodiscard]] auto get_error_string(const uint32_t &native_error_code) const
        -> std::string override;

private:
    static libusb_context *m_libusb_context;
    libusb_device_handle *m_device_handle = nullptr;
    static constexpr uint32_t M_FTDI_VENDOR_ID = 0x0403;
    static constexpr uint32_t M_TRANSFER_TIMEOUT_MSEC = 2000;
    static constexpr uint32_t M_MAX_USB_DESCRIPTOR_LENGTH_BYTES = 255;

    static auto read_descriptor(libusb_device_handle *device_handle, uint8_t descriptor_index)
        -> std::string;
    static auto get_port_path(libusb_device_handle *device_handle) -> std::string;
};

#endif // COMMUNICATION_LIBUSB_H
