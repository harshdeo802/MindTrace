#include "collectors/filesystem_collector.hpp"
#include <spdlog/spdlog.h>

namespace mindtrace {

void FileSystemCollector::start() {
    if (running_.exchange(true)) return;

    spdlog::info("FileSystemCollector starting, watching: {}", watch_dir_.string());
    scan_thread_ = std::thread([this] { scan_loop(); });
}

void FileSystemCollector::stop() {
    if (!running_.exchange(false)) return;

    if (scan_thread_.joinable()) {
        scan_thread_.join();
    }
    spdlog::info("FileSystemCollector stopped");
}

void FileSystemCollector::scan_loop() {
    // Initial scan — mark all existing files
    scan_directory();

    while (running_.load()) {
        std::this_thread::sleep_for(interval_);
        if (!running_.load()) break;
        scan_directory();
    }
}

void FileSystemCollector::scan_directory() {
    if (!std::filesystem::exists(watch_dir_)) return;

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(
                 watch_dir_, std::filesystem::directory_options::skip_permission_denied)) {

            if (!entry.is_regular_file()) continue;

            auto path_str = entry.path().string();
            auto last_write = entry.last_write_time();

            auto it = file_states_.find(path_str);
            if (it == file_states_.end()) {
                // New file discovered
                file_states_[path_str] = last_write;

                auto now = static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count());

                ActivityEvent event;
                event.timestamp = now;
                event.type = EventType::FileOpened;
                event.source = path_str;
                event.content = "File discovered: " + entry.path().filename().string();
                event.metadata["size"] = std::to_string(entry.file_size());
                event.metadata["extension"] = entry.path().extension().string();

                emit(std::move(event));

            } else if (it->second != last_write) {
                // File was modified
                it->second = last_write;

                auto now = static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count());

                ActivityEvent event;
                event.timestamp = now;
                event.type = EventType::FileModified;
                event.source = path_str;
                event.content = "File modified: " + entry.path().filename().string();
                event.metadata["size"] = std::to_string(entry.file_size());
                event.metadata["extension"] = entry.path().extension().string();

                emit(std::move(event));
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("FileSystemCollector scan error: {}", e.what());
    }
}

}  // namespace mindtrace
