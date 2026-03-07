#ifndef MINDTRACE_COLLECTORS_FILESYSTEM_COLLECTOR_HPP
#define MINDTRACE_COLLECTORS_FILESYSTEM_COLLECTOR_HPP

#include "collectors/collector_base.hpp"
#include <filesystem>
#include <thread>
#include <chrono>
#include <unordered_map>

namespace mindtrace {

/// Collector that scans a directory for file changes and emits
/// FileOpened / FileModified events.
class FileSystemCollector : public CollectorBase {
public:
    /// Construct with a directory to watch and a scan interval.
    explicit FileSystemCollector(
        std::filesystem::path watch_dir,
        std::chrono::seconds interval = std::chrono::seconds(5))
        : CollectorBase("FileSystemCollector"),
          watch_dir_(std::move(watch_dir)),
          interval_(interval) {}

    void start() override;
    void stop() override;

    /// Get the directory being watched.
    [[nodiscard]] const std::filesystem::path& watch_dir() const { return watch_dir_; }

private:
    void scan_loop();
    void scan_directory();

    std::filesystem::path watch_dir_;
    std::chrono::seconds interval_;
    std::thread scan_thread_;

    /// Track last-modified times to detect changes.
    std::unordered_map<std::string, std::filesystem::file_time_type> file_states_;
};

}  // namespace mindtrace

#endif  // MINDTRACE_COLLECTORS_FILESYSTEM_COLLECTOR_HPP
