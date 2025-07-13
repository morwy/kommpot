#include "communication_libftdi.h"

#include "kommpot_core.h"
#include "libkommpot.h"
#include "third-party/libftdi/src/ftdi.h"
#include "third-party/libusb-cmake/libusb/libusb/libusb.h"
#include "third-party/spdlog/include/spdlog/spdlog.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

ftdi_context *communication_libftdi::m_ftdi_context = nullptr;

communication_libftdi::communication_libftdi(const kommpot::communication_information &information)
    : kommpot::device_communication(information)
{
    m_type = kommpot::communication_type::LIBFTDI;
}

communication_libftdi::~communication_libftdi()
{
    close();

    if (m_ftdi_context != nullptr)
    {
        ftdi_free(m_ftdi_context);
    }
}

auto communication_libftdi::devices(
    const std::vector<kommpot::device_identification> &identifications)
    -> std::vector<std::shared_ptr<kommpot::device_communication>>
{
    std::vector<std::shared_ptr<kommpot::device_communication>> devices;

    ftdi_context *ftdi_context = ftdi_new();
    if (ftdi_context == nullptr)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "ftdi_new() failed with error");
        return {};
    }

    ftdi_device_list *device_list = nullptr;
    int result_code = ftdi_usb_find_all(ftdi_context, &device_list, 0, 0);
    if (result_code < 0)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "ftdi_usb_find_all() failed with error {} [{}]",
            ftdi_get_error_string(ftdi_context), result_code);
        return {};
    }

    ftdi_device_list *current_device = device_list;
    do
    {
        constexpr size_t max_string_size_bytes = 128;
        std::array<char, max_string_size_bytes> manufacturer = {0};
        std::array<char, max_string_size_bytes> description = {0};
        std::array<char, max_string_size_bytes> serial_number = {0};

        result_code = ftdi_usb_get_strings(ftdi_context, current_device->dev, manufacturer.data(),
            manufacturer.size(), description.data(), description.size(), serial_number.data(),
            serial_number.size());
        if (result_code < 0)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "ftdi_usb_get_strings() failed with error {} [{}]",
                ftdi_get_error_string(ftdi_context), result_code);
            continue;
        }

        libusb_device_descriptor device_description = {0};
        result_code = libusb_get_device_descriptor(current_device->dev, &device_description);
        if (result_code != 0)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                "libusb_get_device_descriptor() failed with error {} [{}]",
                libusb_error_name(result_code), result_code);
            continue;
        }

        for (const auto &identification : identifications)
        {
            /**
             * Filter device by VendorID.
             */
            if (identification.vendor_id != 0x0000 &&
                identification.vendor_id != device_description.idVendor)
            {
                SPDLOG_LOGGER_TRACE(KOMMPOT_LOGGER,
                    "Found device VID{}:PID{}, requested VID{}, skipping.",
                    device_description.idVendor, device_description.idProduct,
                    identification.vendor_id);
                continue;
            }

            /**
             * Filter device by ProductID.
             */
            if (identification.product_id != 0x0000 &&
                identification.product_id != device_description.idProduct)
            {
                SPDLOG_LOGGER_TRACE(KOMMPOT_LOGGER,
                    "Found device VID{}:PID{}, requested PID{}, skipping.",
                    device_description.idVendor, device_description.idProduct,
                    identification.product_id);
                continue;
            }

            kommpot::communication_information information;
            information.name = description.data();
            information.manufacturer = manufacturer.data();
            information.serial_number = serial_number.data();
            information.port = get_port_path(current_device->dev);
            information.vendor_id = device_description.idVendor;
            information.product_id = device_description.idProduct;

            /**
             * Filter device by serial number.
             */
            if (!identification.serial_number.empty() &&
                identification.serial_number != information.serial_number)
            {
                SPDLOG_LOGGER_TRACE(KOMMPOT_LOGGER, "Found device {}, requested {}, skipping.",
                    identification.serial_number, information.serial_number);
                continue;
            }

            /**
             * Filter device by port.
             */
            if (!identification.port.empty() && identification.port != information.port)
            {
                SPDLOG_LOGGER_TRACE(KOMMPOT_LOGGER,
                    "Found device at port {}, requested at port {}, skipping.", identification.port,
                    information.port);
                continue;
            }

            std::shared_ptr<kommpot::device_communication> device =
                std::make_shared<communication_libftdi>(information);
            if (!device)
            {
                SPDLOG_LOGGER_ERROR(
                    KOMMPOT_LOGGER, "std::make_shared() failed creating the device!");
                continue;
            }

            devices.push_back(device);

            break;
        }

    } while ((current_device = current_device->next) != nullptr);

    ftdi_list_free(&device_list);
    ftdi_free(ftdi_context);

    return devices;
}

auto communication_libftdi::open() -> bool
{
    if (m_ftdi_context == nullptr)
    {
        m_ftdi_context = ftdi_new();
        if (m_ftdi_context == nullptr)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "ftdi_new() failed with error");
            return false;
        }

        int result_code = libusb_set_option(
            m_ftdi_context->usb_ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);
        if (result_code != 0)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                "libusb_set_option(LIBUSB_OPTION_LOG_LEVEL) failed with error {} [{}]",
                libusb_error_name(result_code), result_code);
        }

        result_code = libusb_set_option(m_ftdi_context->usb_ctx, LIBUSB_OPTION_LOG_CB, log);
        if (result_code != 0)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                "libusb_set_option(LIBUSB_OPTION_LOG_CB) failed with error {} [{}]",
                libusb_error_name(result_code), result_code);
        }
    }

    int result_code = ftdi_usb_open_desc(m_ftdi_context, communication_libftdi::VENDOR_ID,
        m_information.product_id, nullptr, m_information.serial_number.c_str());
    if (result_code < 0)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "ftdi_usb_open_desc() failed with error {} [{}]",
            ftdi_get_error_string(m_ftdi_context), result_code);
        return false;
    }

    result_code =
        ftdi_set_bitmode(m_ftdi_context, m_configuration.bit_mask, m_configuration.bit_mode);
    if (result_code < 0)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "ftdi_set_bitmode() failed with error {} [{}]",
            ftdi_get_error_string(m_ftdi_context), result_code);
        return false;
    }

    m_is_device_opened = true;

    return true;
}

auto communication_libftdi::is_open() -> bool
{
    return m_is_device_opened;
}

auto communication_libftdi::close() -> void
{
    if (m_ftdi_context == nullptr)
    {
        return;
    }

    int result_code = ftdi_usb_close(m_ftdi_context);
    if (result_code < 0)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "ftdi_usb_close() failed with error {} [{}]",
            ftdi_get_error_string(m_ftdi_context), result_code);
    }

    m_is_device_opened = false;
}

auto communication_libftdi::endpoints() -> std::vector<kommpot::endpoint_information>
{
    std::vector<kommpot::endpoint_information> endpoints;

    libusb_config_descriptor *config_descriptor = nullptr;
    int result_code = libusb_get_active_config_descriptor(
        libusb_get_device(m_ftdi_context->usb_dev), &config_descriptor);
    if (result_code < 0)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
            "libusb_get_active_config_descriptor() failed with error {} [{}]",
            libusb_error_name(result_code), result_code);
        return {};
    }

    for (uint8_t interface_index = 0; interface_index < config_descriptor->bNumInterfaces;
        interface_index++)
    {
        const libusb_interface interface = config_descriptor->interface[interface_index];
        for (int altsetting_index = 0; altsetting_index < interface.num_altsetting;
            altsetting_index++)
        {
            const libusb_interface_descriptor altsetting = interface.altsetting[altsetting_index];
            for (int endpoint_index = 0; endpoint_index < altsetting.bNumEndpoints;
                endpoint_index++)
            {
                const libusb_endpoint_descriptor libusb_endpoint =
                    altsetting.endpoint[endpoint_index];

                kommpot::endpoint_information information;
                information.address = (libusb_endpoint.bEndpointAddress & 0xF);
                information.type = (libusb_endpoint.bEndpointAddress & LIBUSB_ENDPOINT_IN) != 0
                                       ? kommpot::endpoint_type::INPUT
                                       : kommpot::endpoint_type::OUTPUT;
                endpoints.push_back(information);
            }
        }
    }

    return endpoints;
}

auto communication_libftdi::read(
    const kommpot::transfer_configuration &configuration, void *data, size_t size_bytes) -> bool
{
    const int result_code = ftdi_read_pins(m_ftdi_context, reinterpret_cast<uint8_t *>(data));
    if (result_code < 0)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "ftdi_read_pins() failed with error {} [{}]",
            ftdi_get_error_string(m_ftdi_context), result_code);
        return false;
    }

    return true;
}

auto communication_libftdi::write(
    const kommpot::transfer_configuration &configuration, void *data, size_t size_bytes) -> bool
{
    const int result_code =
        ftdi_write_data(m_ftdi_context, reinterpret_cast<uint8_t *>(data), size_bytes);
    if (result_code < 0)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "ftdi_write_data() failed with error {} [{}]",
            ftdi_get_error_string(m_ftdi_context), result_code);
        return false;
    }

    return true;
}

auto communication_libftdi::get_error_string(const uint32_t &native_error_code) const -> std::string
{
    return "";
}

auto communication_libftdi::native_handle() const -> void *
{
    return m_ftdi_context;
}

auto communication_libftdi::get_port_path(libusb_device *device) -> std::string
{
    constexpr uint32_t max_usb_tiers = 7;
    std::vector<uint8_t> port_numbers(max_usb_tiers, 0);
    int result_code = libusb_get_port_numbers(device, port_numbers.data(), port_numbers.size());
    if (result_code <= 0)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "libusb_get_port_numbers() failed with error {} [{}]",
            libusb_error_name(result_code), result_code);
        return {};
    }

    port_numbers.resize(result_code);

    std::ostringstream stream;
    for (size_t port_index = 0; port_index < port_numbers.size(); port_index++)
    {
        stream << std::to_string(port_numbers[port_index]);

        if (port_index != (port_numbers.size() - 1))
        {
            stream << ':';
        }
    }

    return stream.str();
}

auto communication_libftdi::strip_trailing_lf(std::string string) -> std::string
{
    if (!string.empty() && string.back() == '\n')
    {
        string.pop_back();
    }

    return string;
}

auto communication_libftdi::log(
    libusb_context *context, libusb_log_level level, const char *message) -> void
{
    switch (level)
    {
    case LIBUSB_LOG_LEVEL_NONE: {
        return;
    }
    case LIBUSB_LOG_LEVEL_ERROR: {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "[context:{:p}]: {}", reinterpret_cast<void *>(context),
            strip_trailing_lf(message));
        break;
    }
    case LIBUSB_LOG_LEVEL_WARNING: {
        SPDLOG_LOGGER_WARN(KOMMPOT_LOGGER, "[context:{:p}]: {}", reinterpret_cast<void *>(context),
            strip_trailing_lf(message));
        break;
    }
    case LIBUSB_LOG_LEVEL_INFO: {
        SPDLOG_LOGGER_INFO(KOMMPOT_LOGGER, "[context:{:p}]: {}", reinterpret_cast<void *>(context),
            strip_trailing_lf(message));
        break;
    }
    case LIBUSB_LOG_LEVEL_DEBUG: {
        SPDLOG_LOGGER_DEBUG(KOMMPOT_LOGGER, "[context:{:p}]: {}", reinterpret_cast<void *>(context),
            strip_trailing_lf(message));
        break;
    }
    }
}
