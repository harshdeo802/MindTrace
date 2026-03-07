#ifndef MINDTRACE_COMMON_CONFIG_HPP
#define MINDTRACE_COMMON_CONFIG_HPP

#include <string>
#include <filesystem>

namespace mindtrace {

/// Application configuration.
struct Config {
    /// Path to the SQLite database file.
    std::string db_path = "mindtrace.db";

    /// Directory to watch for file changes.
    std::string watch_directory = ".";

    /// Scan interval for file system collector (seconds).
    int scan_interval_seconds = 5;

    /// Number of pipeline worker threads.
    int pipeline_workers = 2;

    /// Session gap threshold for timeline grouping (seconds).
    int session_gap_seconds = 1800;

    /// Maximum search results to return.
    int max_results = 100;

    /// Get the default config directory.
    static std::filesystem::path config_dir() {
        const char* home = std::getenv("USERPROFILE");
        if (!home) home = std::getenv("HOME");
        if (!home) return ".mindtrace";
        return std::filesystem::path(home) / ".mindtrace";
    }

    /// Get the default database path.
    static std::string default_db_path() {
        auto dir = config_dir();
        std::filesystem::create_directories(dir);
        return (dir / "mindtrace.db").string();
    }
};

}  // namespace mindtrace

#endif  // MINDTRACE_COMMON_CONFIG_HPP
