#ifndef HTTP_CONTEXT_H
#define HTTP_CONTEXT_H

#pragma once

class http_context
{
public:
    static auto instance() -> http_context &
    {
        static http_context instance;
        return instance;
    }

    http_context(const http_context &) = delete;
    auto operator=(const http_context &) -> void = delete;

    auto initialize() -> bool;
    auto deinitialize() -> bool;

private:
    bool m_is_initialized = false;

    http_context() = default;
    ~http_context() = default;
};

#endif // HTTP_CONTEXT_H