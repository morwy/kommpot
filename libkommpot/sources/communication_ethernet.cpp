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
    const kommpot::communication_information &information)
    : kommpot::device_communication(information)
{
    m_type = kommpot::communication_type::ETHERNET;
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

    int socket_handle = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_handle < 0)
    {
        SPDLOG_LOGGER_ERROR(
            KOMMPOT_LOGGER, "socket() failed with error {} [{}]", strerror(errno), errno);
        return {};
    }

    if (!set_socket_timeout(socket_handle, M_TRANSFER_TIMEOUT_MSEC))
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "set_socket_timeout() failed");
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
                auto hosts = scan_network(interface, *identification);
                devices.insert(std::end(devices), std::make_move_iterator(std::begin(hosts)),
                    std::make_move_iterator(std::end(hosts)));
            }
            else
            {
                sockaddr_in broadcast_address = {};
                broadcast_address.sin_family = AF_INET;
                broadcast_address.sin_port = htons(identification->port);
                broadcast_address.sin_addr.s_addr = inet_addr(identification->ip.c_str());
            }
        }
    }

#ifdef _WIN32
    closesocket(socket_handle);
    WSACleanup();
#else
    close(socket_handle);
#endif

    return devices;
}

auto communication_ethernet::open() -> bool
{
    return false;
}

auto communication_ethernet::is_open() -> bool
{
    return false;
}

auto communication_ethernet::close() -> void {}

auto communication_ethernet::endpoints() -> std::vector<kommpot::endpoint_information>
{
    std::vector<kommpot::endpoint_information> endpoints;

    return endpoints;
}

auto communication_ethernet::read(
    const kommpot::transfer_configuration &configuration, void *data, size_t size_bytes) -> bool
{
    return false;
}

auto communication_ethernet::write(
    const kommpot::transfer_configuration &configuration, void *data, size_t size_bytes) -> bool
{
    return false;
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
        const auto friendy_name_str =
            std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(adapter->FriendlyName);

        /**
         * @brief skip non-Ethernet interfaces.
         */
        if (adapter->IfType != IF_TYPE_ETHERNET_CSMACD)
        {
            SPDLOG_LOGGER_TRACE(
                KOMMPOT_LOGGER, "Skipping non-Ethernet interface: {}", friendy_name_str);
            continue;
        }

        /**
         * @brief skip disconnected interfaces.
         */
        if (adapter->OperStatus != IfOperStatusUp)
        {
            SPDLOG_LOGGER_TRACE(
                KOMMPOT_LOGGER, "Skipping disconnected interface: {}", friendy_name_str);
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
                KOMMPOT_LOGGER, "Skipping interface without IPv4 address: {}", friendy_name_str);
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
                KOMMPOT_LOGGER, "Skipping interface without IPv4 gateway: {}", friendy_name_str);
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

auto communication_ethernet::is_alive(const ethernet_ipv4_address &ip, int port) -> bool
{
    int socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_handle == INVALID_SOCKET)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "socket() failed with error {}", WSAGetLastError());
        return false;
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.to_string().c_str(), &addr.sin_addr);

    if (!set_socket_timeout(socket_handle, M_TRANSFER_TIMEOUT_MSEC))
    {
        return false;
    }

    int result = connect(socket_handle, (sockaddr *)&addr, sizeof(addr));
    if (result == SOCKET_ERROR)
    {
        result = closesocket(socket_handle);
        if (result == SOCKET_ERROR)
        {
            SPDLOG_LOGGER_TRACE(
                KOMMPOT_LOGGER, "closesocket() failed with error {}", WSAGetLastError());
            return false;
        }

        return false;
    }

    SPDLOG_LOGGER_TRACE(KOMMPOT_LOGGER, "connect() to {}:{} succeed", ip.to_string(), port);

    result = closesocket(socket_handle);
    if (result == SOCKET_ERROR)
    {
        SPDLOG_LOGGER_TRACE(
            KOMMPOT_LOGGER, "closesocket() failed with error {}", WSAGetLastError());
        return false;
    }

    return true;
}

auto communication_ethernet::scan_network(const ethernet_interface_information &interface,
    const kommpot::ethernet_device_identification &identification)
    -> const std::vector<std::shared_ptr<kommpot::device_communication>>
{
    std::mutex mutex;
    std::vector<std::thread> threads;
    std::vector<std::shared_ptr<kommpot::device_communication>> hosts;

    for (int i = 1; i < interface.ipv4_max_hosts - 1; ++i)
    {
        const auto host_ip = ethernet_ipv4_address(interface.ipv4_base_address.to_uint32() + i);

        threads.emplace_back([host_ip, identification, &mutex, &hosts]() {
            if (is_alive(host_ip, identification.port))
            {
                kommpot::communication_information information;

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

auto communication_ethernet::set_socket_timeout(int socket, const uint32_t &timeout_msecs) -> const
    bool
{
    struct timeval timeout = {};

    /**
     * @todo here is difference between Windows and *nix OSes.
     */
    timeout.tv_sec = timeout_msecs / 1000;
    timeout.tv_usec = (timeout_msecs % 1000) * 1000;

    int result = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    if (result != NO_ERROR)
    {
        SPDLOG_LOGGER_ERROR(
            KOMMPOT_LOGGER, "setsockopt(SO_RCVTIMEO) failed with error {}", WSAGetLastError());
        return false;
    }

    result = setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
    if (result != NO_ERROR)
    {
        SPDLOG_LOGGER_ERROR(
            KOMMPOT_LOGGER, "setsockopt(SO_SNDTIMEO) failed with error {}", WSAGetLastError());
        return false;
    }

    return true;
}
