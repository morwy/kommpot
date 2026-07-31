#include "communication_ethernet.h"

#include <communications/ethernet/ethernet_address_factory.h>
#include <communications/ethernet/ethernet_context.h>
#include <communications/ethernet/ethernet_tools.h>
#include <kommpot_core.h>
#include <libkommpot.h>
#include <third-party/spdlog/include/spdlog/spdlog.h>
#include <third-party/spdlog/include/spdlog/stopwatch.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#ifdef _WIN32
// clang-format off
#    include <codecvt>
#    include <winsock2.h>
#    include <ws2tcpip.h>
#    include <iphlpapi.h>
#    pragma comment(lib, "ws2_32.lib")
#    pragma comment(lib, "iphlpapi.lib")

// clang-format on

/**
 * @warning Windows API has a great define "interface".
 */
#    ifdef interface
#        undef interface
#    endif

#else
#    include <arpa/inet.h>
#    include <ifaddrs.h>
#    include <sys/socket.h>
#    include <unistd.h>

#    ifdef __linux__
#        include <netpacket/packet.h>
#    endif

#    ifdef __APPLE__
#        include <net/if_dl.h>
#    endif

#endif

communication_ethernet::communication_ethernet(
    const kommpot::ethernet_device_identification &identification)
    : kommpot::device_communication(identification)
{
    m_type = kommpot::communication_type::ETHERNET;
    m_identification = identification;
}

communication_ethernet::~communication_ethernet()
{
    close();
}

auto communication_ethernet::devices(
    const std::vector<kommpot::device_identification> &identifications)
    -> std::vector<std::shared_ptr<kommpot::device_communication>>
{
    std::vector<std::shared_ptr<kommpot::device_communication>> devices;

    if (!ethernet_context::instance().initialize())
    {
        return {};
    }

    const auto interfaces = get_all_interfaces();
    if (interfaces.empty())
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "No Ethernet interfaces found!");
        return {};
    }

    for (const auto &identification_variant : identifications)
    {
        const auto *identification =
            std::get_if<kommpot::ethernet_device_identification>(&identification_variant);
        if (identification == nullptr)
        {
            SPDLOG_LOGGER_TRACE(
                KOMMPOT_LOGGER, "Provided identification is not Ethernet, skipping.");
            continue;
        }

        for (const auto &interface : interfaces)
        {
            if (identification->ip == "*")
            {
                auto hosts = scan_network_for_hosts(interface.ipv4, *identification);
                devices.insert(std::end(devices), std::make_move_iterator(std::begin(hosts)),
                    std::make_move_iterator(std::end(hosts)));
            }
            else
            {
                auto ip_address_opt = ethernet_address_factory::from_string(identification->ip);
                if (!ip_address_opt)
                {
                    SPDLOG_LOGGER_ERROR(
                        KOMMPOT_LOGGER, "Invalid IP address format: {}", identification->ip);
                    continue;
                }
                auto ip_address = *ip_address_opt;

                kommpot::ethernet_device_identification host_id;
                if (!is_host_reachable(ip_address, identification->port, host_id))
                {
                    continue;
                }

                auto host = std::make_shared<communication_ethernet>(host_id);
                if (host == nullptr)
                {
                    SPDLOG_LOGGER_ERROR(
                        KOMMPOT_LOGGER, "std::make_shared() failed creating the device!");
                    continue;
                }

                if (!is_host_suitable(*identification, host_id))
                {
                    continue;
                }

                devices.push_back(host);
            }
        }
    }

    return devices;
}

auto communication_ethernet::open() -> bool
{
    auto ip_address_opt = ethernet_address_factory::from_string(m_identification.ip);
    if (!ip_address_opt)
    {
        return false;
    }
    auto ip_address = *ip_address_opt;

    if (ip_address == nullptr)
    {
        return false;
    }

    if (!m_socket.initialize(ip_address, m_identification.port, m_identification.protocol))
    {
        return false;
    }

    if (!m_socket.set_timeout(M_TRANSFER_TIMEOUT_MSEC))
    {
        return false;
    }

    return m_socket.connect();
}

auto communication_ethernet::is_open() -> bool
{
    return m_socket.is_connected();
}

auto communication_ethernet::close() -> void
{
    if (m_socket.is_connected())
    {
        if (!m_socket.disconnect())
        {
            SPDLOG_LOGGER_ERROR(
                KOMMPOT_LOGGER, "Socket {} failed to disconnect!", m_socket.to_string());
        }
    }
}

auto communication_ethernet::endpoints() -> std::vector<kommpot::endpoint_information>
{
    std::vector<kommpot::endpoint_information> endpoints;

    auto information = kommpot::endpoint_information();

    information.address = m_identification.port;
    information.type = kommpot::endpoint_type::DUPLEX;

    endpoints.push_back(information);

    return endpoints;
}

auto communication_ethernet::read(
    const kommpot::transfer_configuration &configuration, void *data, size_t size_bytes) -> bool
{
    return m_socket.read(data, size_bytes);
}

auto communication_ethernet::write(
    const kommpot::transfer_configuration &configuration, void *data, size_t size_bytes) -> bool
{
    return m_socket.write(data, size_bytes);
}

auto communication_ethernet::get_error_string(const uint32_t &native_error_code) const
    -> std::string
{
    return "";
}

auto communication_ethernet::native_handle() const -> void *
{
    if (!m_socket.is_connected())
    {
        return nullptr;
    }

    return m_socket.native_handle();
}

auto communication_ethernet::get_all_interfaces()
    -> const std::vector<ethernet_interface_information>
{
    std::vector<ethernet_interface_information> interfaces;

#ifdef _WIN32
    ULONG buffer_size_bytes = 64 * 1024;

    auto deleter = [](IP_ADAPTER_ADDRESSES *ptr) { free(ptr); };
    std::shared_ptr<IP_ADAPTER_ADDRESSES> addresses(
        (IP_ADAPTER_ADDRESSES *)malloc(buffer_size_bytes), deleter);

    if (!addresses)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to allocate memory for adapter addresses.");
        return {};
    }

    const auto family = AF_UNSPEC;
    const auto flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS;

    auto result = GetAdaptersAddresses(family, flags, nullptr, addresses.get(), &buffer_size_bytes);

    if (result == ERROR_BUFFER_OVERFLOW)
    {
        addresses.reset((IP_ADAPTER_ADDRESSES *)malloc(buffer_size_bytes), deleter);
        if (!addresses)
        {
            SPDLOG_LOGGER_ERROR(
                KOMMPOT_LOGGER, "Failed to reallocate memory for adapter addresses.");
            return {};
        }

        result = GetAdaptersAddresses(family, flags, nullptr, addresses.get(), &buffer_size_bytes);
    }

    if (result != NO_ERROR)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "GetAdaptersAddresses() failed with error {} [{}]",
            result, GetLastError());
        return {};
    }

    for (IP_ADAPTER_ADDRESSES *adapter = addresses.get(); adapter != nullptr;
        adapter = adapter->Next)
    {
        const auto friendly_name_str =
            std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(adapter->FriendlyName);

        /**
         * @brief skip non-Ethernet interfaces.
         */
        if (adapter->IfType != IF_TYPE_ETHERNET_CSMACD)
        {
            SPDLOG_LOGGER_TRACE(
                KOMMPOT_LOGGER, "Skipping non-Ethernet interface: {}", friendly_name_str);
            continue;
        }

        /**
         * @brief skip disconnected interfaces.
         */
        if (adapter->OperStatus != IfOperStatusUp)
        {
            SPDLOG_LOGGER_TRACE(
                KOMMPOT_LOGGER, "Skipping disconnected interface: {}", friendly_name_str);
            continue;
        }

        /**
         * @brief fill in the information structure.
         */
        auto interface = ethernet_interface_information();

        interface.adapter_name = adapter->AdapterName;
        interface.interface_name = adapter->FriendlyName;
        interface.description = adapter->Description;
        interface.dns_suffix = adapter->DnsSuffix;

        auto mac_result = ethernet_address_factory::from_array(
            adapter->PhysicalAddress, sizeof(adapter->PhysicalAddress));
        if (!mac_result)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to create MAC address from byte array.");
            continue;
        }
        interface.mac_address = *mac_result;

        /**
         * @brief get current IP addresses.
         */
        SOCKADDR_IN *ipv4_address = nullptr;
        SOCKADDR_IN *ipv6_address = nullptr;

        uint8_t ipv4_prefix_length = 0;
        uint8_t ipv6_prefix_length = 0;

        auto *unicast = adapter->FirstUnicastAddress;
        while (unicast)
        {
            if (ipv4_address == nullptr && unicast->Address.lpSockaddr->sa_family == AF_INET)
            {
                ipv4_address = (SOCKADDR_IN *)unicast->Address.lpSockaddr;
                ipv4_prefix_length = unicast->OnLinkPrefixLength;
            }

            if (ipv6_address == nullptr && unicast->Address.lpSockaddr->sa_family == AF_INET6)
            {
                ipv6_address = (SOCKADDR_IN *)unicast->Address.lpSockaddr;
                ipv6_prefix_length = unicast->OnLinkPrefixLength;
            }

            unicast = unicast->Next;
        }

        if (ipv4_address == nullptr && ipv6_address == nullptr)
        {
            SPDLOG_LOGGER_TRACE(KOMMPOT_LOGGER,
                "Skipping interface without IPv4 and IPv6 address: {}", friendly_name_str);
            continue;
        }

        auto ipv4_addr_result =
            ethernet_address_factory::from_sockaddr_in((SOCKADDR *)ipv4_address);
        bool is_valid_ipv4 = ipv4_addr_result.has_value();
        if (is_valid_ipv4)
            interface.ipv4.address = *ipv4_addr_result;
        if (!is_valid_ipv4)
        {
            SPDLOG_LOGGER_WARN(KOMMPOT_LOGGER, "Failed to create IPv4 address from sockaddr_in.");
        }

        auto ipv6_addr_result =
            ethernet_address_factory::from_sockaddr_in((SOCKADDR *)ipv6_address);
        bool is_valid_ipv6 = ipv6_addr_result.has_value();
        if (is_valid_ipv6)
            interface.ipv6.address = *ipv6_addr_result;
        if (!is_valid_ipv6)
        {
            SPDLOG_LOGGER_WARN(KOMMPOT_LOGGER, "Failed to create IPv6 address from sockaddr_in.");
        }

        if (!is_valid_ipv4 && !is_valid_ipv6)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                "Failed to create both IPv4 and IPv6 addresses from sockaddr_in: {}",
                friendly_name_str);
            continue;
        }

        /**
         * @brief get current IPv4 default gateway.
         */
        SOCKADDR_IN *ipv4_gateway = nullptr;
        SOCKADDR_IN *ipv6_gateway = nullptr;

        auto *gateway = adapter->FirstGatewayAddress;
        while (gateway)
        {
            if (ipv4_gateway == nullptr && gateway->Address.lpSockaddr->sa_family == AF_INET)
            {
                ipv4_gateway = (SOCKADDR_IN *)gateway->Address.lpSockaddr;
            }

            if (ipv6_gateway == nullptr && gateway->Address.lpSockaddr->sa_family == AF_INET6)
            {
                ipv6_gateway = (SOCKADDR_IN *)gateway->Address.lpSockaddr;
            }

            gateway = gateway->Next;
        }

        if (ipv4_gateway == nullptr && ipv6_gateway == nullptr)
        {
            SPDLOG_LOGGER_TRACE(KOMMPOT_LOGGER,
                "Skipping interface without IPv4 and IPv6 gateway: {}", friendly_name_str);
            continue;
        }

        auto ipv4_gw_result = ethernet_address_factory::from_sockaddr_in((SOCKADDR *)ipv4_gateway);
        is_valid_ipv4 = ipv4_gw_result.has_value();
        if (is_valid_ipv4)
            interface.ipv4.gateway = *ipv4_gw_result;
        if (!is_valid_ipv4)
        {
            SPDLOG_LOGGER_WARN(KOMMPOT_LOGGER, "Failed to create IPv4 gateway from sockaddr_in.");
        }

        auto ipv6_gw_result = ethernet_address_factory::from_sockaddr_in((SOCKADDR *)ipv6_gateway);
        is_valid_ipv6 = ipv6_gw_result.has_value();
        if (is_valid_ipv6)
            interface.ipv6.gateway = *ipv6_gw_result;
        if (!is_valid_ipv6)
        {
            SPDLOG_LOGGER_WARN(KOMMPOT_LOGGER, "Failed to create IPv6 gateway from sockaddr_in.");
        }

        if (!is_valid_ipv4 && !is_valid_ipv6)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                "Failed to create both IPv4 and IPv6 gateways from sockaddr_in: {}",
                friendly_name_str);
            continue;
        }

        /**
         * @brief get current IP mask.
         */
        interface.ipv4.mask_prefix = ipv4_prefix_length;
        interface.ipv6.mask_prefix = ipv6_prefix_length;

        auto ipv4_mask_result = ethernet_address_factory::calculate_mask(
            interface.ipv4.address, interface.ipv4.mask_prefix);
        is_valid_ipv4 = ipv4_mask_result.has_value();
        if (is_valid_ipv4)
            interface.ipv4.mask = *ipv4_mask_result;
        if (!is_valid_ipv4)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to calculate IPv4 mask.");
        }

        auto ipv6_mask_result = ethernet_address_factory::calculate_mask(
            interface.ipv6.address, interface.ipv6.mask_prefix);
        is_valid_ipv6 = ipv6_mask_result.has_value();
        if (is_valid_ipv6)
            interface.ipv6.mask = *ipv6_mask_result;
        if (!is_valid_ipv6)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to calculate IPv6 mask.");
        }

        if (!is_valid_ipv4 && !is_valid_ipv6)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                "Failed to calculate both IPv4 and IPv6 masks from sockaddr_in: {}",
                friendly_name_str);
            continue;
        }

        auto ipv4_base_result = ethernet_address_factory::calculate_base_address(
            interface.ipv4.address, interface.ipv4.mask);
        is_valid_ipv4 = ipv4_base_result.has_value();
        if (is_valid_ipv4)
            interface.ipv4.base_address = *ipv4_base_result;
        if (!is_valid_ipv4)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to calculate IPv4 base address.");
        }

        auto ipv6_base_result = ethernet_address_factory::calculate_base_address(
            interface.ipv6.address, interface.ipv6.mask);
        is_valid_ipv6 = ipv6_base_result.has_value();
        if (is_valid_ipv6)
            interface.ipv6.base_address = *ipv6_base_result;
        if (!is_valid_ipv6)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to calculate IPv6 base address.");
        }

        if (!is_valid_ipv4 && !is_valid_ipv6)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                "Failed to calculate both IPv4 and IPv6 base addresses from sockaddr_in: {}",
                friendly_name_str);
            continue;
        }

        auto ipv4_count_result = ethernet_address_factory::calculate_address_count(
            interface.ipv4.address, interface.ipv4.mask_prefix);
        is_valid_ipv4 = ipv4_count_result.has_value();
        if (is_valid_ipv4)
            interface.ipv4.max_hosts = *ipv4_count_result;
        if (!is_valid_ipv4)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to calculate IPv4 max hosts.");
        }

        auto ipv6_count_result = ethernet_address_factory::calculate_address_count(
            interface.ipv6.address, interface.ipv6.mask_prefix);
        is_valid_ipv6 = ipv6_count_result.has_value();
        if (is_valid_ipv6)
            interface.ipv6.max_hosts = *ipv6_count_result;
        if (!is_valid_ipv6)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to calculate IPv6 max hosts.");
        }

        if (!is_valid_ipv4 && !is_valid_ipv6)
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                "Failed to calculate both IPv4 and IPv6 max hosts from sockaddr_in: {}",
                friendly_name_str);
            continue;
        }

        interfaces.push_back(interface);
    }

#else

    struct ifaddrs *adapters_raw = nullptr;
    if (getifaddrs(&adapters_raw) == -1)
    {

        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed getting list of interfaces due to error: {}.",
            ethernet_tools::get_last_error_code_as_string());
        return {};
    }

    auto adapters = std::shared_ptr<ifaddrs>(adapters_raw, [](ifaddrs *ptr) {
        if (ptr != nullptr)
        {
            freeifaddrs(ptr);
        }
    });

    for (auto adapter = adapters.get(); adapter != nullptr; adapter = adapter->ifa_next)
    {
        if (adapter->ifa_addr == nullptr)
        {
            continue;
        }

        /**
         * @brief skip non-Ethernet interfaces.
         */
        if ((strncmp(adapter->ifa_name, "en", 2) != 0 && strncmp(adapter->ifa_name, "eth", 3) != 0))
        {
            SPDLOG_LOGGER_TRACE(
                KOMMPOT_LOGGER, "Skipping non-Ethernet interface: {}", adapter->ifa_name);
            continue;
        }

        /**
         * @brief fill in the information structure.
         */

        /**
         * @brief get current MAC addresses.
         */
#    ifdef __linux__
        if (adapter->ifa_addr->sa_family == AF_PACKET)
#    elif __APPLE__
        if (adapter->ifa_addr->sa_family == AF_LINK)
#    endif
        {
            auto &interface = find_or_create_interface(interfaces, adapter->ifa_name);

#    ifdef __linux__
            struct sockaddr_ll *s = (struct sockaddr_ll *)adapter->ifa_addr;
            if (s->sll_halen == 6)
            {
                SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                    "Interface {} has invalid MAC address length: {}.", adapter->ifa_name,
                    s->sll_halen);
                continue;
            }

            auto mac_result =
                ethernet_address_factory::from_array(s->sll_addr, sizeof(s->sll_addr));
            if (!mac_result)
            {
                SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                    "Failed to convert MAC address from byte array for interface {}.",
                    adapter->ifa_name);
                continue;
            }
            interface.mac_address = *mac_result;
#    elif __APPLE__

            /**
             * @attention On macOS, the sockaddr_dl sdl_data field has interface name appended
             * before the MAC address.
             */
            struct sockaddr_dl *sdl = (struct sockaddr_dl *)adapter->ifa_addr;

            if (sdl->sdl_alen != 6)
            {
                SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                    "Interface {} has invalid MAC address length: {}.", adapter->ifa_name,
                    sdl->sdl_alen);
                continue;
            }

            const uint8_t macIndex = interface.adapter_name.size();

            uint8_t mac[6] = {0};
            memcpy(mac, &sdl->sdl_data[macIndex], 6);

            auto mac_result = ethernet_address_factory::from_array(mac, sizeof(mac));
            if (!mac_result)
            {
                SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
                    "Failed to convert MAC address from byte array for interface {}.",
                    adapter->ifa_name);
                continue;
            }
            interface.mac_address = *mac_result;
#    endif
        }
        /**
         * @brief get current IP addresses.
         */
        else if (adapter->ifa_addr->sa_family == AF_INET)
        {
            auto &interface = find_or_create_interface(interfaces, adapter->ifa_name);

            auto ipv4_addr_result = ethernet_address_factory::from_sockaddr_in(adapter->ifa_addr);
            if (!ipv4_addr_result)
            {
                SPDLOG_LOGGER_ERROR(
                    KOMMPOT_LOGGER, "Failed to create IPv4 address from sockaddr_in.");
                continue;
            }
            interface.ipv4.address = *ipv4_addr_result;

            auto ipv4_mask_result =
                ethernet_address_factory::from_sockaddr_in(adapter->ifa_netmask);
            if (!ipv4_mask_result)
            {
                SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to create IPv4 mask from sockaddr_in.");
                continue;
            }
            interface.ipv4.mask = *ipv4_mask_result;

            auto ipv4_base_result = ethernet_address_factory::calculate_base_address(
                interface.ipv4.address, interface.ipv4.mask);
            if (!ipv4_base_result)
            {
                SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to calculate IPv4 base address.");
                continue;
            }
            interface.ipv4.base_address = *ipv4_base_result;

            auto ipv4_prefix_result =
                ethernet_address_factory::calculate_mask_prefix(interface.ipv4.mask);
            if (!ipv4_prefix_result)
            {
                SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to calculate IPv4 mask prefix.");
                continue;
            }
            interface.ipv4.mask_prefix = *ipv4_prefix_result;

            auto ipv4_count_result = ethernet_address_factory::calculate_address_count(
                interface.ipv4.address, interface.ipv4.mask_prefix);
            if (!ipv4_count_result)
            {
                SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to calculate IPv4 max hosts.");
                continue;
            }
            interface.ipv4.max_hosts = *ipv4_count_result;
        }
        else if (adapter->ifa_addr->sa_family == AF_INET6)
        {
            auto &interface = find_or_create_interface(interfaces, adapter->ifa_name);

            auto ipv6_addr_result = ethernet_address_factory::from_sockaddr_in(adapter->ifa_addr);
            if (!ipv6_addr_result)
            {
                SPDLOG_LOGGER_ERROR(
                    KOMMPOT_LOGGER, "Failed to create IPv6 address from sockaddr_in.");
                continue;
            }
            interface.ipv6.address = *ipv6_addr_result;

            auto ipv6_mask_result =
                ethernet_address_factory::from_sockaddr_in(adapter->ifa_netmask);
            if (!ipv6_mask_result)
            {
                SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to create IPv6 mask from sockaddr_in.");
                continue;
            }
            interface.ipv6.mask = *ipv6_mask_result;

            auto ipv6_base_result = ethernet_address_factory::calculate_base_address(
                interface.ipv6.address, interface.ipv6.mask);
            if (!ipv6_base_result)
            {
                SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to calculate IPv6 base address.");
                continue;
            }
            interface.ipv6.base_address = *ipv6_base_result;

            auto ipv6_prefix_result =
                ethernet_address_factory::calculate_mask_prefix(interface.ipv6.mask);
            if (!ipv6_prefix_result)
            {
                SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to calculate IPv6 mask prefix.");
                continue;
            }
            interface.ipv6.mask_prefix = *ipv6_prefix_result;

            auto ipv6_count_result = ethernet_address_factory::calculate_address_count(
                interface.ipv6.address, interface.ipv6.mask_prefix);
            if (!ipv6_count_result)
            {
                SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Failed to calculate IPv6 max hosts.");
                continue;
            }
            interface.ipv6.max_hosts = *ipv6_count_result;
        }
        else
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Unsupported address family {} for interface {}.",
                adapter->ifa_addr->sa_family, adapter->ifa_name);
            continue;
        }
    }

    for (auto it = interfaces.begin(); it != interfaces.end();)
    {
        if (it->mac_address.empty() || (it->ipv4.address == nullptr && it->ipv6.address == nullptr))
        {
            SPDLOG_LOGGER_TRACE(
                KOMMPOT_LOGGER, "Removing incomplete interface: {}.", it->adapter_name);
            it = interfaces.erase(it);
        }
        else
        {
            ++it;
        }
    }

#endif

    return interfaces;
}

auto communication_ethernet::find_or_create_interface(
    std::vector<ethernet_interface_information> &interfaces, const std::string &interface_name)
    -> ethernet_interface_information &
{
    for (auto &interface : interfaces)
    {
        if (interface.adapter_name == interface_name)
        {
            return interface;
        }
    }

    auto &interface = interfaces.emplace_back();
    interface.adapter_name = interface_name;

    return interfaces.back();
}

auto communication_ethernet::is_host_reachable(
    const std::shared_ptr<ethernet_ip_address> ip_address, const uint16_t port,
    kommpot::ethernet_device_identification &information) -> bool
{
    /**
     * @todo shall we also check the UDP here?
     */
    const auto protocol = kommpot::ethernet_protocol_type::TCP;

    auto socket = ethernet_socket();

    if (!socket.initialize(ip_address, port, protocol))
    {
        return false;
    }

    if (!socket.set_timeout(M_TRANSFER_TIMEOUT_MSEC))
    {
        return false;
    }

    if (!socket.connect())
    {
        SPDLOG_LOGGER_TRACE(KOMMPOT_LOGGER, "Host '{}' is not reachable via '{}'.",
            ip_address->to_string(), kommpot::ethernet_protocol_type_to_string(protocol));
        return false;
    }

    SPDLOG_LOGGER_TRACE(KOMMPOT_LOGGER, "Host '{}' is reachable via '{}'.", socket.to_string(),
        kommpot::ethernet_protocol_type_to_string(protocol));

    information.name = socket.hostname();
    information.ip = ip_address->to_string();
    information.mac = socket.mac_address().to_string();
    information.port = port;
    information.protocol = protocol;

    if (!socket.disconnect())
    {
        return false;
    }

    return true;
}

auto communication_ethernet::is_host_suitable(
    const kommpot::ethernet_device_identification &search_id,
    const kommpot::ethernet_device_identification &host_id) -> bool
{
    /**
     * @attention port and protocol is covered by is_host_reachable().
     */

    /**
     * Check name.
     */
    const bool is_name_match = is_wildcard_match(search_id.name, host_id.name);
    if (!is_name_match)
    {
        SPDLOG_LOGGER_TRACE(KOMMPOT_LOGGER, "Host {}/'{}' does not match search name '{}'!",
            host_id.ip, host_id.name, search_id.name);
        return false;
    }

    /**
     * Check IP address.
     */
    const bool is_ip_match = is_wildcard_match(search_id.ip, host_id.ip);
    if (!is_ip_match)
    {
        SPDLOG_LOGGER_TRACE(
            KOMMPOT_LOGGER, "Host '{}' does not match search IP '{}'!", host_id.ip, search_id.ip);
        return false;
    }

    /**
     * Check MAC address.
     */
    const bool is_mac_match = is_wildcard_match(search_id.mac, host_id.mac);
    if (!is_mac_match)
    {
        SPDLOG_LOGGER_TRACE(KOMMPOT_LOGGER, "Host {}/'{}' does not match search MAC '{}'!",
            host_id.ip, host_id.mac, search_id.mac);
        return false;
    }

    return true;
}

auto communication_ethernet::scan_network_for_hosts(const ethernet_network_information &network,
    const kommpot::ethernet_device_identification &identification)
    -> const std::vector<std::shared_ptr<kommpot::device_communication>>
{
    std::mutex mutex;
    std::vector<std::shared_ptr<kommpot::device_communication>> hosts;

    spdlog::stopwatch stopwatch;
    std::atomic<uint64_t> scanned_hosts = 0;

    /**
     * @brief hosts to scan live in the half-open range [first_index, last_index).
     */
    const uint64_t first_index = 1;
    const uint64_t last_index = (network.max_hosts > 1) ? (network.max_hosts - 1) : first_index;
    std::atomic<uint64_t> next_index = first_index;

    /**
     * @brief a fixed pool of workers keeps pulling the next address to scan, so a fast host never
     * waits for a slow one to time out before the next address is picked up.
     */
    const uint64_t total_hosts = last_index - first_index;
    const uint32_t worker_count =
        static_cast<uint32_t>(std::min<uint64_t>(M_MAX_CONCURRENT_SEARCH_THREADS, total_hosts));

    auto worker = [&]() {
        while (true)
        {
            const uint64_t host_index = next_index.fetch_add(1, std::memory_order_relaxed);
            if (host_index >= last_index)
            {
                break;
            }

            auto new_address_opt =
                ethernet_address_factory::calculate_new_address(network.base_address, host_index);
            if (!new_address_opt)
            {
                continue;
            }

            auto new_address = *new_address_opt;
            scanned_hosts.fetch_add(1, std::memory_order_relaxed);

            kommpot::ethernet_device_identification host_id;
            if (!is_host_reachable(new_address, identification.port, host_id))
            {
                continue;
            }

            if (!is_host_suitable(identification, host_id))
            {
                continue;
            }

            auto host = std::make_shared<communication_ethernet>(host_id);
            if (!host)
            {
                SPDLOG_LOGGER_ERROR(
                    KOMMPOT_LOGGER, "std::make_shared() failed creating the device!");
                continue;
            }

            std::lock_guard<std::mutex> lock(mutex);
            hosts.push_back(host);
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(worker_count);
    for (uint32_t worker_index = 0; worker_index < worker_count; ++worker_index)
    {
        workers.emplace_back(worker);
    }

    for (auto &worker_thread : workers)
    {
        worker_thread.join();
    }

    SPDLOG_LOGGER_INFO(KOMMPOT_LOGGER,
        "scan_network_for_hosts(): scanned {} host(s), found {} device(s) in {:.3} seconds.",
        scanned_hosts.load(), hosts.size(), stopwatch);

    return hosts;
}

/**
 * @author Jack Handy (jakkhandy@hotmail.com)
 * @link https://www.codeproject.com/Articles/1088/Wildcard-string-compare-globbing-
 */
auto communication_ethernet::is_wildcard_match(const std::string &pattern, const std::string &value)
    -> bool
{
    const char *pattern_ptr = pattern.c_str();
    const char *value_ptr = value.c_str();

    const char *cp = nullptr;
    const char *mp = nullptr;

    while ((*value_ptr) && (*pattern_ptr != '*'))
    {
        if ((*pattern_ptr != *value_ptr) && (*pattern_ptr != '?'))
        {
            return false;
        }

        pattern_ptr++;
        value_ptr++;
    }

    while (*value_ptr)
    {
        if (*pattern_ptr == '*')
        {
            if (!*++pattern_ptr)
            {
                return true;
            }

            mp = pattern_ptr;
            cp = value_ptr + 1;
        }
        else if ((*pattern_ptr == *value_ptr) || (*pattern_ptr == '?'))
        {
            pattern_ptr++;
            value_ptr++;
        }
        else
        {
            pattern_ptr = mp;
            value_ptr = cp++;
        }
    }

    while (*pattern_ptr == '*')
    {
        pattern_ptr++;
    }

    return !*pattern_ptr;
}
