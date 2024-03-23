#include "communication_libusb.h"

#include "third-party/spdlog/include/spdlog/spdlog.h"

#include <sstream>

libusb_context *communication_libusb::m_libusb_context = nullptr;

communication_libusb::communication_libusb(const kommpot::communication_information &information)
    : kommpot::device_communication(information)
{}

auto communication_libusb::get_available_devices()
    -> std::vector<std::unique_ptr<kommpot::device_communication>>
{
    std::vector<std::unique_ptr<kommpot::device_communication>> devices;

    int result_code = libusb_init(nullptr);
    if (result_code != 0)
    {
        spdlog::error(
            "libusb_init() failed with error {} [{}]", libusb_error_name(result_code), result_code);
        return {};
    }

    libusb_device **device_list = nullptr;
    ssize_t device_list_count = libusb_get_device_list(nullptr, &device_list);

    for (size_t device_index = 0; device_index < device_list_count; ++device_index)
    {
        libusb_device_descriptor device_description = {0};
        result_code = libusb_get_device_descriptor(device_list[device_index], &device_description);
        if (result_code != 0)
        {
            spdlog::error("libusb_get_device_descriptor() failed with error {} [{}]",
                libusb_error_name(result_code), result_code);
            continue;
        }

        /**
         * Skip FTDI devices, since they will be handled by ftdi_access_io.
         */
        if (device_description.idVendor == M_FTDI_VENDOR_ID)
        {
            continue;
        }

        libusb_device_handle *device_handle = nullptr;
        result_code = libusb_open(device_list[device_index], &device_handle);
        if (result_code != 0)
        {
            spdlog::error("libusb_open() failed with error {} [{}]", libusb_error_name(result_code),
                result_code);
            continue;
        }

        const std::string manufacturer =
            read_descriptor(device_handle, device_description.iManufacturer);
        const std::string product = read_descriptor(device_handle, device_description.iProduct);
        const std::string serial_number =
            get_serial_number(device_handle, device_description.iSerialNumber);

        libusb_close(device_handle);

        kommpot::communication_information information;
        information.name = product;
        information.serial_number = serial_number;

        std::unique_ptr<kommpot::device_communication> device =
            std::make_unique<communication_libusb>(information);
        if (!device)
        {
            spdlog::error("std::make_unique() failed creating the device!");
            continue;
        }

        devices.push_back(std::move(device));
    }

    libusb_free_device_list(device_list, static_cast<int>(device_list_count));
    libusb_exit(nullptr);

    return devices;
}

auto communication_libusb::open() -> bool
{
    libusb_device **device_list = nullptr;

    if (m_libusb_context == nullptr)
    {
        int result_code = libusb_init(&m_libusb_context);
        if (result_code != 0)
        {
            spdlog::error("libusb_init() failed with error {} [{}]", libusb_error_name(result_code),
                result_code);
        }
    }

    ssize_t device_list_count = libusb_get_device_list(m_libusb_context, &device_list);

    for (size_t device_index = 0; device_index < device_list_count; ++device_index)
    {
        libusb_device *usb_device = device_list[device_index];

        libusb_device_descriptor device_description = {0};
        int result_code = libusb_get_device_descriptor(usb_device, &device_description);
        if (result_code != 0)
        {
            spdlog::error("libusb_get_device_descriptor() failed with error {} [{}]",
                libusb_error_name(result_code), result_code);
            continue;
        }

        libusb_device_handle *device_handle = nullptr;
        result_code = libusb_open(usb_device, &device_handle);
        if (result_code != 0)
        {
            spdlog::error("libusb_open() failed with error {} [{}]", libusb_error_name(result_code),
                result_code);
            continue;
        }

        const std::string device_serial_number =
            get_serial_number(device_handle, device_description.iSerialNumber);
        if (m_information.serial_number != device_serial_number)
        {
            libusb_close(device_handle);
            continue;
        }

        m_device_handle = device_handle;

        libusb_free_device_list(device_list, static_cast<int>(device_list_count));

        return true;
    }

    libusb_free_device_list(device_list, static_cast<int>(device_list_count));

    return false;
}

auto communication_libusb::is_open() -> bool
{
    return m_device_handle != nullptr;
}

void communication_libusb::close()
{
    if (m_device_handle == nullptr)
    {
        return;
    }

    libusb_close(m_device_handle);
    m_device_handle = nullptr;
}

auto communication_libusb::read(const int &endpoint_address, void *data, size_t size_bytes) -> bool
{
    int size_bytes_written = 0;
    auto *data_ptr = reinterpret_cast<unsigned char *>(data);
    int result_code = libusb_bulk_transfer(m_device_handle, endpoint_address, data_ptr, size_bytes,
        &size_bytes_written, M_TRANSFER_TIMEOUT_MSEC);
    if (result_code < 0)
    {
        spdlog::error("libusb_bulk_transfer() failed with error {} [{}]",
            libusb_error_name(result_code), result_code);
        return false;
    }

    return true;
}

auto communication_libusb::write(const int &endpoint_address, void *data, size_t size_bytes) -> bool
{
    int size_bytes_written = 0;
    auto *data_ptr = reinterpret_cast<unsigned char *>(data);
    int result_code = libusb_bulk_transfer(m_device_handle, endpoint_address, data_ptr, size_bytes,
        &size_bytes_written, M_TRANSFER_TIMEOUT_MSEC);
    if (result_code < 0)
    {
        spdlog::error("libusb_bulk_transfer() failed with error {} [{}]",
            libusb_error_name(result_code), result_code);
        return false;
    }

    return true;
}

auto communication_libusb::get_error_string(const uint32_t &native_error_code) const -> std::string
{
    return libusb_error_name(static_cast<int>(native_error_code));
}

std::string communication_libusb::read_descriptor(
    libusb_device_handle *device_handle, uint8_t descriptor_index)
{
    std::string descriptor_text = "";
    descriptor_text.resize(M_MAX_USB_DESCRIPTOR_LENGTH_BYTES);

    const int result_code = libusb_get_string_descriptor_ascii(device_handle, descriptor_index,
        reinterpret_cast<unsigned char *>(descriptor_text.data()), descriptor_text.size());
    if (result_code <= 0)
    {
        spdlog::error("libusb_get_string_descriptor_ascii() failed with error {} [{}]",
            libusb_error_name(result_code), result_code);
        descriptor_text = "";
    }
    else
    {
        descriptor_text.resize(result_code);
    }

    return descriptor_text;
}

auto communication_libusb::get_serial_number(
    libusb_device_handle *device_handle, uint8_t serial_number_descriptor_index) -> std::string
{
    std::string device_serial_number = "";
    if (serial_number_descriptor_index != 0)
    {
        device_serial_number = read_descriptor(device_handle, serial_number_descriptor_index);
    }
    else
    {
        constexpr uint32_t max_usb_tiers = 7;
        std::vector<uint8_t> port_numbers(max_usb_tiers, 0);
        int result_code = libusb_get_port_numbers(
            libusb_get_device(device_handle), port_numbers.data(), port_numbers.size());
        if (result_code <= 0)
        {
            spdlog::error("libusb_get_port_numbers() failed with error {} [{}]",
                libusb_error_name(result_code), result_code);
        }
        else
        {
            port_numbers.resize(result_code);

            std::ostringstream stream;
            stream << std::string(M_ARTIFICIAL_ID_PREFIX.begin(), M_ARTIFICIAL_ID_PREFIX.end() - 1);

            for (size_t port_index = 0; port_index < port_numbers.size(); port_index++)
            {
                stream << std::to_string(port_numbers[port_index]);

                if (port_index != (port_numbers.size() - 1))
                {
                    stream << ':';
                }
            }

            device_serial_number = stream.str();
        }
    }

    return device_serial_number;
}
