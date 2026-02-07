#include <communications/ethernet/ethernet_tools.h>

#include <cstring>

#ifdef _WIN32
// clang-format off
#    include <winsock2.h>
// clang-format on
#endif

auto ethernet_tools::get_last_error_code_as_string() -> const std::string
{
    const auto error_code = get_error_code();
    return get_error_code_as_string(error_code) + " (code " + std::to_string(error_code) + ")";
}

auto ethernet_tools::get_error_code() -> const int32_t
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

auto ethernet_tools::get_error_code_as_string(const int32_t &error_code) -> const std::string
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
