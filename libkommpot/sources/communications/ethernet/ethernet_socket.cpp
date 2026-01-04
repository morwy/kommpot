#include <communications/ethernet/ethernet_socket.h>

#include <communications/ethernet/ethernet_context.h>
#include <communications/ethernet/ethernet_tools.h>
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
#    include <net/route.h>
#    include <netdb.h>
#    include <netinet/if_ether.h>
#    include <netinet/in.h>
#    include <sys/socket.h>
#    include <unistd.h>

#    ifdef __APPLE__
#        include <net/if_dl.h>
#        include <sys/sysctl.h>
#    endif

#    ifndef SA_SIZE
#        define SA_SIZE(sa)                                                                        \
            (((sa) == NULL)                                                                        \
                    ? sizeof(long)                                                                 \
                    : ((sa)->sin_len == 0 ? sizeof(long)                                           \
                                          : (1 + (((sa)->sin_len - 1) | (sizeof(long) - 1)))))
#    endif

#endif

ethernet_socket::ethernet_socket()
{
    SPDLOG_LOGGER_TRACE(
        KOMMPOT_LOGGER, "Socket {}: constructed object.", static_cast<void *>(this));
}

ethernet_socket::~ethernet_socket()
{
    if (is_connected())
    {
        if (!disconnect())
        {
            SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} / {}: failed to disconnect!",
                static_cast<void *>(this), to_string());
        }
    }

    SPDLOG_LOGGER_TRACE(KOMMPOT_LOGGER, "Socket {} / {}: destructed object.",
        static_cast<void *>(this), to_string());
}

auto ethernet_socket::initialize(const std::shared_ptr<ethernet_ip_address> ip_address,
    const uint16_t &port, const kommpot::ethernet_protocol_type &protocol) -> const bool
{
    if (!ethernet_context::instance().initialize())
    {
        return false;
    }

    m_protocol = protocol;
    m_ip_address = ip_address;
    m_port = port;

    if (dynamic_cast<ethernet_ipv4_address *>(m_ip_address.get()))
    {
        m_ip_family = AF_INET;
    }
    else if (dynamic_cast<ethernet_ipv6_address *>(m_ip_address.get()))
    {
        m_ip_family = AF_INET6;
    }
    else
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
            "Socket {}: Provided IP {} address is not IPv4 or IPv6.", static_cast<void *>(this),
            m_ip_address->to_string());
        return false;
    }

    /**
     * @todo add support for more protocols.
     */
    m_handle = socket(m_ip_family,
        (m_protocol == kommpot::ethernet_protocol_type::TCP) ? SOCK_STREAM : SOCK_DGRAM,
        (m_protocol == kommpot::ethernet_protocol_type::TCP) ? IPPROTO_TCP : IPPROTO_UDP);
    if (m_handle == ETH_INVALID_SOCKET)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} / {}: not created due to error: {}.",
            static_cast<void *>(this), to_string(),
            ethernet_tools::get_last_error_code_as_string());
        return false;
    }

    return true;
}

auto ethernet_socket::connect() -> const bool
{
    if (is_connected())
    {
        SPDLOG_LOGGER_DEBUG(KOMMPOT_LOGGER, "Socket {} / {}: already connected.",
            static_cast<void *>(this), to_string());
        return true;
    }

    sockaddr_in address = {};
    address.sin_family = m_ip_family;
    address.sin_port = htons(m_port);
    inet_pton(m_ip_family, m_ip_address->to_string().c_str(), &address.sin_addr);

    const auto result = ::connect(m_handle, (sockaddr *)&address, sizeof(address));
    if (result == ETH_SOCKET_ERROR)
    {
        SPDLOG_LOGGER_DEBUG(KOMMPOT_LOGGER, "Socket {} / {}: failed to connect due to error: {}.",
            static_cast<void *>(this), to_string(),
            ethernet_tools::get_last_error_code_as_string());
        close_socket();
        return false;
    }

    if (!read_out_hostname(m_hostname))
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} / {}: failed to read out hostname.",
            static_cast<void *>(this), to_string());
        close_socket();
        return false;
    }

    if (!read_out_mac_address(m_mac_address))
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} / {}: failed to read out MAC address.",
            static_cast<void *>(this), to_string());
        close_socket();
        return false;
    }

    SPDLOG_LOGGER_DEBUG(KOMMPOT_LOGGER, "Socket {} / {}: connected successfully.",
        static_cast<void *>(this), to_string());

    m_is_connected = true;

    return true;
}

auto ethernet_socket::disconnect() -> const bool
{
    if (!is_connected())
    {
        SPDLOG_LOGGER_DEBUG(KOMMPOT_LOGGER, "Socket {} / {}: not connected.",
            static_cast<void *>(this), to_string());
        return true;
    }

    return close_socket();
}

auto ethernet_socket::is_connected() const -> const bool
{
    return m_handle != ETH_INVALID_SOCKET && m_is_connected;
}

auto ethernet_socket::read(void *data, size_t size_bytes) const -> const bool
{
    if (!is_connected())
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} / {}: not connected.",
            static_cast<const void *>(this), to_string());
        return false;
    }

    if (data == nullptr)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
            "Socket {} / {}: read() called with nullptr data pointer.",
            static_cast<const void *>(this), to_string());
        return false;
    }

    if (size_bytes == 0)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} / {}: read() called with zero size.",
            static_cast<const void *>(this), to_string());
        return false;
    }

    const auto bytes_received = recv(m_handle, static_cast<char *>(data), size_bytes, 0);
    if (bytes_received == ETH_SOCKET_ERROR)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} / {}: failed to read data due to error: {}.",
            static_cast<const void *>(this), to_string(),
            ethernet_tools::get_last_error_code_as_string());
        return false;
    }

    return true;
}

auto ethernet_socket::write(void *data, size_t size_bytes) const -> const bool
{
    if (!is_connected())
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} / {}: not connected.",
            static_cast<const void *>(this), to_string());
        return false;
    }

    if (data == nullptr)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
            "Socket {} / {}: write() called with nullptr data pointer.",
            static_cast<const void *>(this), to_string());
        return false;
    }

    if (size_bytes == 0)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} / {}: write() called with zero size.",
            static_cast<const void *>(this), to_string());
        return false;
    }

    const auto bytes_sent = send(m_handle, static_cast<const char *>(data), size_bytes, 0);
    if (bytes_sent == ETH_SOCKET_ERROR)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
            "Socket {} / {}: failed to write data due to error: {}.",
            static_cast<const void *>(this), to_string(),
            ethernet_tools::get_last_error_code_as_string());
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
            "Socket {} / {}: setsockopt(SO_RCVTIMEO) failed with error: {}.",
            static_cast<void *>(this), to_string(),
            ethernet_tools::get_last_error_code_as_string());
        return false;
    }

    result = setsockopt(m_handle, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
    if (result == ETH_SOCKET_ERROR)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
            "Socket {} / {}: setsockopt(SO_SNDTIMEO) failed with error: {}.",
            static_cast<void *>(this), to_string(),
            ethernet_tools::get_last_error_code_as_string());
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

auto ethernet_socket::native_handle() const -> void *
{
    return reinterpret_cast<void *>(m_handle);
}

auto ethernet_socket::to_string() const -> const std::string
{
    return fmt::format("{}:{} ({})", m_ip_address ? m_ip_address->to_string() : "NULL", m_port,
        kommpot::ethernet_protocol_type_to_string(m_protocol));
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
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
            "Socket {} / {}: failed to disconnect due to error: {}.", static_cast<void *>(this),
            to_string(), ethernet_tools::get_last_error_code_as_string());
        return false;
    }

    m_handle = ETH_INVALID_SOCKET;
    m_is_connected = false;

    SPDLOG_LOGGER_DEBUG(KOMMPOT_LOGGER, "Socket {} / {}: disconnected successfully.",
        static_cast<void *>(this), to_string());

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
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} / {}: getpeername() failed with error: {}.",
            static_cast<void *>(this), to_string(),
            ethernet_tools::get_last_error_code_as_string());
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
            "Socket {} / {}: getnameinfo() failed with error {} (code {}).",
            static_cast<void *>(this), to_string(), gai_strerror(result), result);
    }

    hostname = host_buffer;

    return true;
}

auto ethernet_socket::read_out_mac_address(ethernet_mac_address &mac_address) -> const bool
{
#ifdef _WIN32

    IPAddr connected_ip_address = {};

    int result = InetPton(m_ip_family, m_ip_address->to_string().c_str(), &connected_ip_address);
    if (result != 1)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "Socket {} / {}: InetPton() failed with error: {}.",
            static_cast<void *>(this), to_string(),
            ethernet_tools::get_last_error_code_as_string());
        return false;
    }

    BYTE mac_address_bytes[6] = {0};
    ULONG mac_address_length = 6;

    result = SendARP(connected_ip_address, 0, mac_address_bytes, &mac_address_length);
    if (result != NO_ERROR)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
            "Socket {} / {}: SendARP() failed with error: {} (code {}).", static_cast<void *>(this),
            to_string(), ethernet_tools::get_error_code_as_string(result), result);
        return false;
    }

    mac_address = ethernet_mac_address(mac_address_bytes);

#elif defined __linux__

    return false;

#elif defined __APPLE__

    struct in_addr ip_address = {};
    if (inet_pton(AF_INET, m_ip_address->to_string().c_str(), &ip_address) != 1)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
            "Socket {} / {}: inet_pton() failed to convert IP address.", static_cast<void *>(this),
            to_string());
        return false;
    }

    constexpr const uint8_t name_size_bytes = 6;

    int name[name_size_bytes] = {};
    name[0] = CTL_NET;
    name[1] = PF_ROUTE;
    name[2] = 0;
    name[3] = AF_INET;
    name[4] = NET_RT_FLAGS;
    name[5] = RTF_LLINFO;

    size_t buffer_size_bytes = 0;
    if (sysctl(name, name_size_bytes, NULL, &buffer_size_bytes, NULL, 0) == -1)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
            "Socket {} / {}: sysctl() failed to get size with error: {}.",
            static_cast<void *>(this), to_string(),
            ethernet_tools::get_last_error_code_as_string());
        return false;
    }

    std::vector<char> buffer(buffer_size_bytes);
    if (sysctl(name, name_size_bytes, buffer.data(), &buffer_size_bytes, NULL, 0) == -1)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER,
            "Socket {} / {}: sysctl() failed to get data with error: {}.",
            static_cast<void *>(this), to_string(),
            ethernet_tools::get_last_error_code_as_string());
        return false;
    }

    char *next = buffer.data();
    char *end = buffer.data() + buffer_size_bytes;

    while (next < end)
    {
        struct rt_msghdr *rtm = (struct rt_msghdr *)next;
        struct sockaddr_inarp *sin = (struct sockaddr_inarp *)(rtm + 1);
        struct sockaddr_dl *sdl = (struct sockaddr_dl *)((char *)sin + SA_SIZE(sin));

        if (sin->sin_addr.s_addr == ip_address.s_addr && sdl->sdl_alen)
        {
            unsigned char *mac = (unsigned char *)LLADDR(sdl);
            mac_address = ethernet_mac_address(mac);

            return true;
        }

        next += rtm->rtm_msglen;
    }

    return false;

#endif

    return true;
}
