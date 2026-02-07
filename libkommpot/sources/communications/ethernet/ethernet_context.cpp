#include <communications/ethernet/ethernet_context.h>

#include <kommpot_core.h>

#ifdef _WIN32
// clang-format off
#    include <winsock2.h>
#    pragma comment(lib, "ws2_32.lib")
// clang-format on
#endif

auto ethernet_context::initialize() -> bool
{
    if (m_is_initialized)
    {
        return true;
    }

#ifdef _WIN32
    static WSADATA wsa_data = {};
    const int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != NO_ERROR)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "WSAStartup() failed with error {}", result);
        return false;
    }
#endif

    m_is_initialized = true;

    return true;
}

auto ethernet_context::deinitialize() -> bool
{
#ifdef _WIN32
    const int result = WSACleanup();
    if (result != NO_ERROR)
    {
        SPDLOG_LOGGER_ERROR(KOMMPOT_LOGGER, "WSACleanup() failed with error {}", WSAGetLastError());
        return false;
    }
#endif

    m_is_initialized = false;

    return true;
}
