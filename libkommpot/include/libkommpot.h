#ifndef LIBKOMMPOT_H
#define LIBKOMMPOT_H

#pragma once

#include "export_definitions.h"

#include <any>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace kommpot {
/**
 * @brief structure containing version of kommpot library.
 */
struct EXPORTED version
{
public:
    version(const uint8_t major, const uint8_t minor, const uint8_t micro, const uint8_t nano,
        std::string git_hash);

    [[nodiscard]] auto major() const noexcept -> uint8_t;
    [[nodiscard]] auto minor() const noexcept -> uint8_t;
    [[nodiscard]] auto micro() const noexcept -> uint8_t;
    [[nodiscard]] auto nano() const noexcept -> uint8_t;

    /**
     * @brief gets short hash of last Git commit
     * @return string
     */
    [[nodiscard]] auto git_hash() const noexcept -> std::string;

    /**
     * @brief creates string based on structure parameters
     * @return string in format major.minor.micro.nano
     */
    [[nodiscard]] auto to_string() const noexcept -> std::string;

private:
    uint8_t m_major = 0;
    uint8_t m_minor = 0;
    uint8_t m_micro = 0;
    uint8_t m_nano = 0;

    /**
     * @brief states short hash of last Git commit
     */
    std::string m_git_hash = "";
};

/**
 * @brief gets current version of kommpot library.
 * @return version structure.
 */
auto EXPORTED get_version() noexcept -> version;

/**
 * @brief structure containing various parameters.
 */
struct dynamic_parameters
{
public:
    template<typename T>
    void set(const std::string &key, T value)
    {
        data[key] = value;
    }

    template<typename T>
    T get(const std::string &key, const T &defaultValue = T{}) const
    {
        auto it = data.find(key);
        if (it != data.end())
        {
            try
            {
                return std::any_cast<T>(it->second);
            }
            catch (const std::bad_any_cast &)
            {
                return defaultValue;
            }
        }

        return defaultValue;
    }

    bool exists(const std::string &key) const
    {
        return data.count(key) > 0;
    }

private:
    std::unordered_map<std::string, std::any> data;
};

/**
 * @brief states types of endpoint.
 */
enum class endpoint_type : uint8_t
{
    UNKNOWN = 0,
    INPUT = 1,
    OUTPUT = 2
};

/**
 * @brief describes parameters of endpoint.
 */
struct endpoint_information
{
public:
    endpoint_type type = endpoint_type::UNKNOWN;
    int address = 0;

    dynamic_parameters parameters;
};

/**
 * @brief states types of communications.
 */
enum class communication_type
{
    UNKNOWN = 0,
    LIBUSB = 1,
    LIBFTDI = 2
};

/**
 * @brief gets communication_type as string.
 * @return communication_type as string.
 */
auto EXPORTED communication_type_to_string(const communication_type &type) noexcept -> std::string;

/**
 * @brief states specific configuration required for opening the device communication.
 */
struct communication_configuration
{
    /**
     * FTDI connection settings
     */
    uint8_t bit_mode = 0;
    uint8_t bit_mask = 0;
};

/**
 * @brief describes parameters of device communication.
 */
struct communication_information
{
    std::string name = "";
    std::string manufacturer = "";
    std::string serial_number = "";
    std::string port = "";
    uint16_t vendor_id = 0x0000;
    uint16_t product_id = 0x0000;
};

struct communication_error
{
    uint32_t code = 0;
    std::string message = "";
};

/**
 * @brief describes any identification parameters of device.
 */
struct device_identification
{
    /**
     * @category general identification parameters.
     * @warning unreliable for unique detection, some devices may have identical serial numbers.
     */
    std::string serial_number = "";

    /**
     * @category USB identification parameters.
     * @attention use 0x0000 value as alternative to wildcard symbol.
     */
    uint16_t vendor_id = 0x0000;
    uint16_t product_id = 0x0000;

    /**
     * reliable for unique detection.
     */
    std::string port = "";
};

class EXPORTED device_communication
{
public:
    explicit device_communication(communication_information information);
    virtual ~device_communication() = default;

    /**
     * @warning states class is non-copyable.
     */
    device_communication(const device_communication &obj) = delete;
    auto operator=(const device_communication &obj) -> device_communication & = delete;

    /**
     * @warning states class is non-movable.
     */
    device_communication(device_communication &&obj) = delete;
    auto operator=(device_communication &&obj) -> device_communication & = delete;

    /**
     * @brief sets specific configuration that has to be applied to device_communication object.
     * @param configuration as communication_configuration structure.
     * {@link communication_configuration communication_configuration}
     */
    void set_configuration(const communication_configuration &configuration)
    {
        m_is_custom_configuration_set = true;
        m_configuration = configuration;
    }

    /**
     * @brief returns current device_communication object configuration.
     * @return configuration as communication_configuration structure.
     * {@link communication_configuration communication_configuration}
     */
    [[nodiscard]] virtual auto configuration() const -> communication_configuration
    {
        return m_configuration;
    }

    /**
     * @brief returns current device_communication object information.
     * @return information as communication_information structure.
     * {@link communication_information communication_information}
     */
    [[nodiscard]] virtual auto information() const -> communication_information
    {
        return m_information;
    }

    /**
     * @brief opens device communication.
     * @return true if opened successfully, false if any error happened.
     */
    virtual auto open() -> bool = 0;

    /**
     * @brief states if device is already opened.
     * @return true if device is opened, false if not.
     */
    virtual auto is_open() -> bool = 0;

    /**
     * @brief closes opened device, has no effect on not opened devices.
     */
    virtual void close() = 0;

    /**
     * @brief returns list of endpoints available on the device.
     * @return information as endpoint_information structure.
     */
    virtual auto endpoints() -> std::vector<endpoint_information> = 0;

    /**
     * @brief reads data from specified endpoint.
     * @param endpoint_address states address as int.
     * @param data states buffer to which read data will be written.
     * @param size_bytes states max buffer size.
     * @return true if read was successful, false if any error happened.
     */
    virtual auto read(const endpoint_information &endpoint, void *data, size_t size_bytes)
        -> bool = 0;

    /**
     * @brief writes data to specified endpoint.
     * @param endpoint_address states address as int.
     * @param data states buffer which will be written.
     * @param size_bytes states max buffer size.
     * @return true if write was successful, false if any error happened.
     */
    virtual auto write(const endpoint_information &endpoint, void *data, size_t size_bytes)
        -> bool = 0;

    /**
     * @brief get human-readable error text from native error code.
     * @param native_error_code as uint32_t.
     * @return error text as std::string.
     */
    [[nodiscard]] virtual auto get_error_string(const uint32_t &native_error_code) const
        -> std::string = 0;

    /**
     * @brief provides access to native library's device handle.
     * @return pointer to device handle, or nullptr.
     */
    [[nodiscard]] virtual auto native_handle() const -> void * = 0;

    /**
     * @brief states currently used communication type.
     * @return enum class value.
     */
    [[nodiscard]] auto type() const -> communication_type;

protected:
    bool m_is_custom_configuration_set = false;
    communication_configuration m_configuration;
    communication_type m_type = communication_type::UNKNOWN;
    communication_information m_information;
};

/**
 * Provides list of devices according to specified device_id.
 * Returns all devices if device_id is not specified.
 *
 * @param device_id.
 * @return std::vector of devices.
 */
auto EXPORTED devices(const std::vector<device_identification> &identifications = {})
    -> std::vector<std::unique_ptr<kommpot::device_communication>>;

class spdlog_initializer
{
public:
    spdlog_initializer();
};
static spdlog_initializer gs_initializer;
} // namespace kommpot

#endif // LIBKOMMPOT_H
