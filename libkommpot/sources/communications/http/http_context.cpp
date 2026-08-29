#include <communications/http/http_context.h>

#include <kommpot_core.h>

#include <curl/curl.h>

auto http_context::initialize() -> bool
{
    if (m_is_initialized)
    {
        return true;
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
    {
        SPDLOG_LOGGER_CRITICAL(KOMMPOT_LOGGER, "curl_global_init() failed!");
        return false;
    }

    m_is_initialized = true;

    return true;
}

auto http_context::deinitialize() -> bool
{
    if (!m_is_initialized)
    {
        return true;
    }

    curl_global_cleanup();
    m_is_initialized = false;

    return true;
}