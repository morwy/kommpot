#ifndef ETHERNET_CONTEXT_H
#define ETHERNET_CONTEXT_H

#pragma once

class ethernet_context
{
public:
    static auto instance() -> ethernet_context &
    {
        static ethernet_context instance;
        return instance;
    }

    ethernet_context(const ethernet_context &) = delete;
    auto operator=(const ethernet_context &) -> void = delete;

    auto initialize() -> bool;
    auto deinitialize() -> bool;

private:
    bool m_is_initialized = false;

    ethernet_context() = default;
    ~ethernet_context() = default;
};

#endif // ETHERNET_CONTEXT_H
