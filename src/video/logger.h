#pragma once

#include <format>
#include <print>

namespace {
bool is_log_enabled = false;
}

namespace logger {

inline void enable_log() {
  is_log_enabled = true;
}

inline void disable_log() {
  is_log_enabled = false;
}

template <class... Args>
void log(const std::format_string<Args...> format, Args &&...args) {
  if (is_log_enabled) {
    std::println(stdout, format, std::forward<Args>(args)...);
  }
}

}  // namespace logger
