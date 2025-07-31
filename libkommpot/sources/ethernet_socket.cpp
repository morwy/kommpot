#include <ethernet_socket.h>

#include <kommpot_core.h>

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
#    include <netdb.h>
#    include <sys/socket.h>
#    include <unistd.h>
#endif

ethernet_socket::ethernet_socket(const ethernet_ipv4_address &ip_address, const uint16_t &port,
    const ethernet_protocol_type &protocol)
    : m_protocol(protocol)
    , m_ip_address(ip_address)
    , m_port(port)
{
    SPDLOG_LOGGER_TRACE(
        KOMMPOT_LOGGER, "Socket {} constructed object {}.", to_string(), static_cast<void *>(this));

    /**
     * @todo add support for IPv6 protocol (AF_INET6).
     */
    m_handle =
        socket(AF_INET, (m_protocol == ethernet_protocol_type::TCP) ? SOCK_STREAM : SOCK_DGRAM,
            (m_protocol == ethernet_protocol_type::TCP) ? IPPROTO_TCP : IPPROTO_UDP);
    if (m_handle == ETH_INVALID_SOCKET)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} is not created due to error: {}.",
            to_string(), get_last_error_code_as_string());
    }
}

ethernet_socket::~ethernet_socket()
{
    if (!is_connected())
    {
        return;
    }

    if (!disconnect())
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} failed to disconnect!", to_string());
    }

    SPDLOG_LOGGER_TRACE(
        KOMMPOT_LOGGER, "Socket {} destructed object {}.", to_string(), static_cast<void *>(this));
}

auto ethernet_socket::connect() -> const bool
{
    if (is_connected())
    {
        SPDLOG_LOGGER_DEBUG(KOMMPOT_LOGGER, "Socket {} is already connected.", to_string());
        return true;
    }

    /**
     * @todo add support for IPv6 protocol (AF_INET6).
     */
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(m_port);
    inet_pton(AF_INET, m_ip_address.to_string().c_str(), &address.sin_addr);

    const auto result = ::connect(m_handle, (sockaddr *)&address, sizeof(address));
    if (result == ETH_SOCKET_ERROR)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} failed to connect due to error: {}.",
            to_string(), get_last_error_code_as_string());
        close_socket();
        return false;
    }

    if (!read_out_hostname(m_hostname))
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} failed to read out hostname.", to_string());
        close_socket();
        return false;
    }

    if (!read_out_mac_address(m_mac_address))
    {
        SPDLOG_LOGGER_ERROR(
            KOMMPOT_LOGGER, "Socket {} failed to read out MAC address.", to_string());
        close_socket();
        return false;
    }

    SPDLOG_LOGGER_DEBUG(KOMMPOT_LOGGER, "Socket {} connected successfully.", to_string());

    m_is_connected = true;

    return true;
}

auto ethernet_socket::disconnect() -> const bool
{
    if (!is_connected())
    {
        SPDLOG_LOGGER_DEBUG(KOMMPOT_LOGGER, "Socket {} is not connected.", to_string());
        return true;
    }

    return close_socket();
}

auto ethernet_socket::is_connected() const -> const bool
{
    return m_handle != ETH_INVALID_SOCKET && m_is_connected;
}

auto ethernet_socket::read(void *data, size_t size_bytes) const -> bool
{
    if (!is_connected())
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} is not connected.", to_string());
        return false;
    }

    if (data == nullptr)
    {
        SPDLOG_LOGGER_ERROR(
            KOMMPOT_LOGGER, "Socket {} read() called with nullptr data pointer.", to_string());
        return false;
    }

    if (size_bytes == 0)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} read() called with zero size.", to_string());
        return false;
    }

    const auto bytes_received = recv(m_handle, static_cast<char *>(data), size_bytes, 0);
    if (bytes_received == ETH_SOCKET_ERROR)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} failed to read data due to error: {}.",
            to_string(), get_last_error_code_as_string());
        return false;
    }

    return true;
}

auto ethernet_socket::write(void *data, size_t size_bytes) const -> bool
{
    if (!is_connected())
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} is not connected.", to_string());
        return false;
    }

    if (data == nullptr)
    {
        SPDLOG_LOGGER_ERROR(
            KOMMPOT_LOGGER, "Socket {} write() called with nullptr data pointer.", to_string());
        return false;
    }

    if (size_bytes == 0)
    {
        SPDLOG_LOGGER_ERROR(
            KOMMPOT_LOGGER, "Socket {} write() called with zero size.", to_string());
        return false;
    }

    const auto bytes_sent = send(m_handle, static_cast<const char *>(data), size_bytes, 0);
    if (bytes_sent == ETH_SOCKET_ERROR)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} failed to write data due to error: {}.",
            to_string(), get_last_error_code_as_string());
        return false;
    }

    return true;
}

auto ethernet_socket::set_timeout(const uint32_t &timeout_msecs) -> const bool
{
    /**
     * @attention please note the difference between Windows and *nix OSes here.
     */
#ifdef _WIN32
    const DWORD timeout = timeout_msecs;
#else
    struct timeval timeout = {};
    timeout.tv_sec = timeout_msecs / 1000;
    timeout.tv_usec = (timeout_msecs % 1000) * 1000;
#endif

    auto result = setsockopt(m_handle, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    if (result == ETH_SOCKET_ERROR)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
            "Socket {} setsockopt(SO_RCVTIMEO) failed with error: {}.", to_string(),
            get_last_error_code_as_string());
        return false;
    }

    result = setsockopt(m_handle, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
    if (result == ETH_SOCKET_ERROR)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
            "Socket {} setsockopt(SO_SNDTIMEO) failed with error: {}.", to_string(),
            get_last_error_code_as_string());
        return false;
    }

    return true;
}

auto ethernet_socket::hostname() const -> const std::string
{
    return m_hostname;
}

auto ethernet_socket::mac_address() const -> const ethernet_mac_address
{
    return m_mac_address;
}

auto ethernet_socket::to_string() const -> const std::string
{
    return fmt::format("{}:{} ({})", m_ip_address.to_string(), m_port,
        (m_protocol == ethernet_protocol_type::TCP) ? "TCP" : "UDP");
}

auto ethernet_socket::close_socket() -> const bool
{
    /**
     * @attention please note the difference between Windows and *nix OSes here.
     */
#ifdef _WIN32
    const auto result = closesocket(m_handle);
#else
    const auto result = close(m_handle);
#endif

    if (result == ETH_SOCKET_ERROR)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} failed to disconnect due to error: {}.",
            to_string(), get_last_error_code_as_string());
        return false;
    }

    m_handle = ETH_INVALID_SOCKET;
    m_is_connected = false;

    SPDLOG_LOGGER_DEBUG(KOMMPOT_LOGGER, "Socket {} disconnected successfully.", to_string());

    return true;
}

auto ethernet_socket::read_out_hostname(std::string &hostname) -> const bool
{
    char host_buffer[NI_MAXHOST] = {0};
    char service_buffer[NI_MAXSERV] = {0};

    sockaddr_storage peer_address = {};
    socklen_t address_length_bytes = sizeof(peer_address);

    int32_t result = getpeername(m_handle, (sockaddr *)&peer_address, &address_length_bytes);
    if (result == ETH_SOCKET_ERROR)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} getpeername() failed with error: {}.",
            to_string(), get_last_error_code_as_string());
        return false;
    }

    /**
     * @attention getnameinfo() has its own error codes that are different to standard socket ones.
     */
    result = getnameinfo((sockaddr *)&peer_address, address_length_bytes, host_buffer,
        sizeof(host_buffer), service_buffer, sizeof(service_buffer), 0);
    if (result != 0)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
            "Socket {} getnameinfo() failed with error {} (code {}).", to_string(),
            gai_strerror(result), result);
    }

    hostname = host_buffer;

    return true;
}

auto ethernet_socket::read_out_mac_address(ethernet_mac_address &mac_address) -> const bool
{
#ifdef _WIN32

    IPAddr connected_ip_address = {};

    /**
     * @todo add support for IPv6 protocol (AF_INET6).
     */
    int result = InetPton(AF_INET, m_ip_address.to_string().c_str(), &connected_ip_address);
    if (result != 1)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} InetPton() failed with error: {}.",
            to_string(), get_last_error_code_as_string());
        return false;
    }

    BYTE mac_address_bytes[6] = {0};
    ULONG mac_address_length = 6;

    result = SendARP(connected_ip_address, 0, mac_address_bytes, &mac_address_length);
    if (result != NO_ERROR)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} SendARP() failed with error: {} (code {}).",
            to_string(), get_error_code_as_string(result), result);
        return false;
    }

    mac_address = ethernet_mac_address(mac_address_bytes);

#else

    return false;

#endif

    return true;
}

auto ethernet_socket::get_last_error_code_as_string() -> const std::string
{
    const auto error_code = get_error_code();
    return get_error_code_as_string(error_code) + " (code " + std::to_string(error_code) + ")";
}

auto ethernet_socket::get_error_code() -> const int32_t
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

auto ethernet_socket::get_error_code_as_string(const int32_t &error_code) -> const std::string
{
#ifdef _WIN32
    char *buffer = nullptr;

    const auto size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buffer, 0, nullptr);

    std::string message = "";
    if (size > 0 && buffer != nullptr)
    {
        message.assign(buffer, size);

        /**
         * @attention removing trailing newlines.
         */
        while (!message.empty() && (message.back() == '\n' || message.back() == '\r'))
        {
            message.pop_back();
        }
    }
    else
    {
        message = "Unknown error code " + std::to_string(error_code);
    }

    if (buffer != nullptr)
    {
        LocalFree(buffer);
    }

    return message;
#else
    return std::strerror(error_code);
#endif
}
