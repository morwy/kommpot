#ifndef COMMUNICATION_LIBFTDI_H
#define COMMUNICATION_LIBFTDI_H

#pragma once

#include "libkommpot.h"

#include "third-party/libftdi/src/ftdi.h"

class communication_libftdi : public kommpot::device_communication
{
public:
    static constexpr uint32_t VENDOR_ID = 0x0403;

    communication_libftdi(const kommpot::communication_information &information);
    ~communication_libftdi();

    static auto get_available_devices(const kommpot::device_identification &identification)
        -> std::vector<std::unique_ptr<kommpot::device_communication>>;

    auto open() -> bool override;
    auto is_open() -> bool override;
    void close() override;

    auto endpoints() -> std::vector<kommpot::endpoint_information> override;

    auto read(const int &endpoint_address, void *data, size_t size_bytes) -> bool override;
    auto write(const int &endpoint_address, void *data, size_t size_bytes) -> bool override;

    auto get_error_string(const uint32_t &native_error_code) const -> std::string override;

    virtual auto native_handle() const -> void * override;

private:
    static ftdi_context *m_ftdi_context;
    bool m_is_device_opened = false;
    static constexpr uint32_t M_TRANSFER_TIMEOUT_MSEC = 2000;

    static auto get_port_path(libusb_device *device) -> std::string;
};

#endif // COMMUNICATION_LIBFTDI_H
