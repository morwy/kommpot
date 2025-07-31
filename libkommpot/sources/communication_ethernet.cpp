#include "communication_ethernet.h"

#include "kommpot_core.h"
#include "libkommpot.h"
#include "third-party/spdlog/include/spdlog/spdlog.h"

#include <codecvt>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
// clang-format off
#    include <winsock2.h>
#    include <ws2tcpip.h>
#    include <iphlpapi.h>
#    pragma comment(lib, "ws2_32.lib")
#    pragma comment(lib, "iphlpapi.lib")
// clang-format on
#else
#    include <arpa/inet.h>
#    include <sys/socket.h>
#    include <unistd.h>
#endif

communication_ethernet::communication_ethernet(
    const kommpot::ethernet_device_identification &identification)
    : kommpot::device_communication(identification)
    , m_socket(identification.ip, identification.port, ethernet_protocol_type::TCP)
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

#ifdef _WIN32
    WSADATA wsa_data = {};
    const int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != NO_ERROR)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "WSAStartup() failed with error {}", result);
        return {};
    }
#endif

    const auto interfaces = get_all_interfaces();
    if (interfaces.empty())
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "No Ethernet interfaces found.");
        return {};
    }

    for (const auto &identification_variant : identifications)
    {
        for (const auto &interface : interfaces)
        {
            const auto *identification =
                std::get_if<kommpot::ethernet_device_identification>(&identification_variant);
            if (identification == nullptr)
            {
                SPDLOG_LOGGER_TRACE(
                    KOMMPOT_LOGGER, "Provided identification is not Ethernet, skipping.");
                continue;
            }

            if (identification->ip == "0.0.0.0")
            {
                auto hosts = scan_network_for_hosts(interface, *identification);
                devices.insert(std::end(devices), std::make_move_iterator(std::begin(hosts)),
                    std::make_move_iterator(std::end(hosts)));
            }
            else
            {
                kommpot::ethernet_device_identification information;
                if (is_host_reachable(identification->ip, identification->port, information))
                {
                    std::shared_ptr<kommpot::device_communication> host =
                        std::make_shared<communication_ethernet>(information);
                    if (!host)
                    {
                        SPDLOG_LOGGER_ERROR(
                            KOMMPOT_LOGGER, "std::make_shared() failed creating the device!");
                        continue;
                    }

                    devices.push_back(host);
                }
            }
        }
    }

#ifdef _WIN32
    WSACleanup();
#endif

    return devices;
}

auto communication_ethernet::open() -> bool
{
    return m_socket.connect();
}

auto communication_ethernet::is_open() -> bool
{
    return m_socket.is_connected();
}

auto communication_ethernet::close() -> void
{
    if (!m_socket.disconnect())
    {
        SPDLOG_LOGGER_ERROR(
            KOMMPOT_LOGGER, "Socket {} failed to disconnect!", m_socket.to_string());
    }
}

auto communication_ethernet::endpoints() -> std::vector<kommpot::endpoint_information>
{
    std::vector<kommpot::endpoint_information> endpoints;

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
    return nullptr;
}

#ifdef _WIN32

ethernet_ipv4_address to_ipv4_address(const SOCKADDR_IN *addr)
{
    ethernet_ipv4_address address;

    const uint32_t ip = ntohl(addr->sin_addr.S_un.S_addr);
    for (int i = 0; i < 4; ++i)
    {
        address.value.at(3 - i) = (ip >> (8 * i)) & 0xFF;
    }

    return address;
}

#else

#endif

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
         * @brief get current IPv4 address.
         */
        SOCKADDR_IN *ipv4_address = nullptr;
        auto *unicast = adapter->FirstUnicastAddress;
        while (unicast)
        {
            if (unicast->Address.lpSockaddr->sa_family == AF_INET)
            {
                ipv4_address = (SOCKADDR_IN *)unicast->Address.lpSockaddr;
                break;
            }

            unicast = unicast->Next;
        }

        if (!ipv4_address)
        {
            SPDLOG_LOGGER_TRACE(
                KOMMPOT_LOGGER, "Skipping interface without IPv4 address: {}", friendly_name_str);
            continue;
        }

        /**
         * @brief get current IPv4 mask.
         */
        const uint8_t prefix_length = unicast->OnLinkPrefixLength;
        const uint32_t mask = (prefix_length == 0) ? 0 : (~0u << (32 - prefix_length));

        /**
         * @brief get current IPv4 default gateway.
         */
        SOCKADDR_IN *ipv4_gateway = nullptr;
        auto *gateway = adapter->FirstGatewayAddress;
        while (gateway)
        {
            if (gateway->Address.lpSockaddr->sa_family == AF_INET)
            {
                ipv4_gateway = (SOCKADDR_IN *)gateway->Address.lpSockaddr;
                break;
            }

            gateway = gateway->Next;
        }

        if (!ipv4_gateway)
        {
            SPDLOG_LOGGER_TRACE(
                KOMMPOT_LOGGER, "Skipping interface without IPv4 gateway: {}", friendly_name_str);
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

        interface.ipv4_address = to_ipv4_address(ipv4_address);
        interface.ipv4_gateway = to_ipv4_address(ipv4_gateway);
        interface.ipv4_mask = ethernet_ipv4_address(mask);
        interface.ipv4_mask_prefix = prefix_length;
        interface.ipv4_base_address = ethernet_ipv4_address(
            interface.ipv4_address.to_uint32() & interface.ipv4_mask.to_uint32());
        interface.ipv4_max_hosts = 1 << (32 - prefix_length);
        interface.mac_address = ethernet_mac_address(adapter->PhysicalAddress);

        interfaces.push_back(interface);
    }

#else

#endif

    return interfaces;
}

auto communication_ethernet::is_host_reachable(const ethernet_ipv4_address &ip, const uint16_t port,
    kommpot::ethernet_device_identification &information) -> bool
{
    auto socket = ethernet_socket(ip, port, ethernet_protocol_type::TCP);

    if (!socket.set_timeout(M_TRANSFER_TIMEOUT_MSEC))
    {
        return false;
    }

    if (!socket.connect())
    {
        return false;
    }

    SPDLOG_LOGGER_TRACE(KOMMPOT_LOGGER, "connect() to host {} succeed", socket.to_string());

    information.name = socket.hostname();
    information.ip = ip.to_string();
    information.mac = socket.mac_address().to_string();
    information.port = port;

    if (!socket.disconnect())
    {
        return false;
    }

    return true;
}

auto communication_ethernet::scan_network_for_hosts(const ethernet_interface_information &interface,
    const kommpot::ethernet_device_identification &identification)
    -> const std::vector<std::shared_ptr<kommpot::device_communication>>
{
    std::mutex mutex;
    std::vector<std::thread> threads;
    std::vector<std::shared_ptr<kommpot::device_communication>> hosts;

    for (uint32_t i = 1; i < interface.ipv4_max_hosts - 1; ++i)
    {
        const auto host_ip = ethernet_ipv4_address(interface.ipv4_base_address.to_uint32() + i);

        threads.emplace_back([host_ip, identification, &mutex, &hosts]() {
            kommpot::ethernet_device_identification information;
            if (is_host_reachable(host_ip, identification.port, information))
            {
                std::shared_ptr<kommpot::device_communication> host =
                    std::make_shared<communication_ethernet>(information);
                if (!host)
                {
                    SPDLOG_LOGGER_ERROR(
                        KOMMPOT_LOGGER, "std::make_shared() failed creating the device!");
                    return;
                }

                std::lock_guard<std::mutex> lock(mutex);
                hosts.push_back(host);
            }
        });

        if (threads.size() >= M_MAX_CONCURRENT_SEARCH_THREADS)
        {
            for (auto &thread : threads)
            {
                thread.join();
            }

            threads.clear();
        }
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    return hosts;
}
