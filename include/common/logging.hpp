#ifndef MINDTRACE_COMMON_LOGGING_HPP
#define MINDTRACE_COMMON_LOGGING_HPP

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>

namespace mindtrace {

/// Initialize the logging system.
inline void init_logging(spdlog::level::level_enum level = spdlog::level::info) {
    auto console = spdlog::stdout_color_mt("mindtrace");
    spdlog::set_default_logger(console);
    spdlog::set_level(level);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
}

}  // namespace mindtrace

#endif  // MINDTRACE_COMMON_LOGGING_HPP
