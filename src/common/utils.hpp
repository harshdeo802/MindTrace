#ifndef MINDTRACE_COMMON_UTILS_HPP
#define MINDTRACE_COMMON_UTILS_HPP

#include <string>
#include <cstdint>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace mindtrace {

/// Format a unix timestamp to a human-readable string.
inline std::string format_timestamp(uint64_t timestamp) {
    auto time = static_cast<time_t>(timestamp);
    std::tm tm_val{};
#ifdef _WIN32
    localtime_s(&tm_val, &time);
#else
    localtime_r(&time, &tm_val);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

/// Format a unix timestamp to time-only string (HH:MM).
inline std::string format_time_only(uint64_t timestamp) {
    auto time = static_cast<time_t>(timestamp);
    std::tm tm_val{};
#ifdef _WIN32
    localtime_s(&tm_val, &time);
#else
    localtime_r(&time, &tm_val);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%H:%M");
    return oss.str();
}

/// Get current unix timestamp.
inline uint64_t current_timestamp() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
}

/// Truncate a string to max_len, appending "..." if truncated.
inline std::string truncate(const std::string& str, size_t max_len = 60) {
    if (str.size() <= max_len) return str;
    return str.substr(0, max_len - 3) + "...";
}

}  // namespace mindtrace

#endif  // MINDTRACE_COMMON_UTILS_HPP
