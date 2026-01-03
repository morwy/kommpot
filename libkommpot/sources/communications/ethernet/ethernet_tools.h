#ifndef ETHERNET_TOOLS_H
#define ETHERNET_TOOLS_H

#pragma once

#include <stdint.h>
#include <string>

namespace ethernet_tools {
/**
 * @attention unifying error code types under standard int32_t.
 */
[[nodiscard]] auto get_last_error_code_as_string() -> const std::string;
[[nodiscard]] auto get_error_code() -> const int32_t;
[[nodiscard]] auto get_error_code_as_string(const int32_t &error_code) -> const std::string;
}; // namespace ethernet_tools

#endif // ETHERNET_TOOLS_H
