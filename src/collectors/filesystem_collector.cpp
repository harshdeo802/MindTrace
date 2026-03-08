#include "collectors/filesystem_collector.hpp"
#include <spdlog/spdlog.h>
#include <unordered_set>
#include <algorithm>
#include <cctype>

namespace mindtrace {

// Helper to convert string to lowercase
static std::string to_lower_ext(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

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
        std::error_code ec;
        auto it = std::filesystem::recursive_directory_iterator(
            watch_dir_, std::filesystem::directory_options::skip_permission_denied, ec);
        auto end = std::filesystem::recursive_directory_iterator();

        // Maintain allowed text, code, document, and media extensions
        static const std::unordered_set<std::string> allowed_extensions = {
            ".txt", ".md", ".pdf", ".doc", ".docx", ".rtf", ".csv", ".xlsx", ".pptx",
            ".cpp", ".hpp", ".h", ".c", ".py", ".js", ".ts", ".html", ".css", ".json",
            ".png", ".jpg", ".jpeg", ".gif", ".mp4", ".mov", ".flv"
        };
        
        static const std::unordered_set<std::string> ignored_directories = {
            "appdata", "cache", "logs", "temp", "tmp", "node_modules", "build"
        };

        while (it != end && running_.load()) {
            if (ec) {
                // Skip problematic directories (e.g., junctions, locked folders)
                it.increment(ec);
                continue;
            }

            try {
                const auto& entry = *it;

                // Eagerly skip known cache folders and hidden directories (starting with '.')
                if (entry.is_directory(ec) && !ec) {
                    std::string dir_name = to_lower_ext(entry.path().filename().string());
                    if (dir_name.length() > 0 && dir_name[0] == '.') {
                        it.disable_recursion_pending();
                    } else if (ignored_directories.count(dir_name) > 0) {
                        it.disable_recursion_pending();
                    }
                    it.increment(ec);
                    continue;
                }

                if (!entry.is_regular_file(ec) || ec) {
                    it.increment(ec);
                    continue;
                }

                std::string ext = to_lower_ext(entry.path().extension().string());
                if (allowed_extensions.count(ext) == 0) {
                    // Extension is not an explicitly tracked human representation format
                    it.increment(ec);
                    continue;
                }

                auto path_str = entry.path().string();
                auto last_write = entry.last_write_time(ec);
                if (ec) {
                    it.increment(ec);
                    continue;
                }

                auto state_it = file_states_.find(path_str);
                if (state_it == file_states_.end()) {
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
                    
                    std::error_code size_ec;
                    auto fsize = entry.file_size(size_ec);
                    if (!size_ec) event.metadata["size"] = std::to_string(fsize);
                    event.metadata["extension"] = entry.path().extension().string();

                    emit(std::move(event));

                } else if (state_it->second != last_write) {
                    // File was modified
                    state_it->second = last_write;

                    auto now = static_cast<uint64_t>(
                        std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count());

                    ActivityEvent event;
                    event.timestamp = now;
                    event.type = EventType::FileModified;
                    event.source = path_str;
                    event.content = "File modified: " + entry.path().filename().string();
                    
                    std::error_code size_ec;
                    auto fsize = entry.file_size(size_ec);
                    if (!size_ec) event.metadata["size"] = std::to_string(fsize);
                    event.metadata["extension"] = entry.path().extension().string();

                    emit(std::move(event));
                }
            } catch (...) {
                // Ignore any individual file read exceptions to keep daemon alive
            }

            it.increment(ec);
        }
    } catch (const std::exception& e) {
        spdlog::error("FileSystemCollector scan error: {}", e.what());
    }
}

}  // namespace mindtrace
