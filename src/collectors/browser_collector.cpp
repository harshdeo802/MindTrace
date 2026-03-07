#include "collectors/browser_collector.hpp"
#include <spdlog/spdlog.h>
#include <sqlite3.h>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

namespace mindtrace {

BrowserCollector::BrowserCollector() : CollectorBase("BrowserCollector") {
    // Determine Chrome History path on Windows
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        chrome_history_path_ = std::filesystem::path(path) / "Google" / "Chrome" / "User Data" / "Default" / "History";
    }
#else
    // Linux/Mac fallback (could add specific logic later)
    chrome_history_path_ = "";
#endif
    
    // Choose a temp location
    temp_history_path_ = std::filesystem::temp_directory_path() / "mindtrace_chrome_history.sqlite";
}

BrowserCollector::~BrowserCollector() {
    stop();
}

void BrowserCollector::start() {
    if (running_.exchange(true)) return;

    if (chrome_history_path_.empty() || !std::filesystem::exists(chrome_history_path_)) {
        spdlog::warn("BrowserCollector: Chrome History file not found at {}", chrome_history_path_.string());
        // Start anyway, maybe it gets created later
    }

    // Set initial timestamp to now (so we don't index years of old history)
    last_visit_time_ = get_chrome_timestamp(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()
    );

    poll_thread_ = std::thread([this]() { poll_loop(); });
    spdlog::info("BrowserCollector started (Polling Chrome History)");
}

void BrowserCollector::stop() {
    if (!running_.exchange(false)) return;

    if (poll_thread_.joinable()) {
        poll_thread_.join();
    }
    spdlog::info("BrowserCollector stopped");
}

void BrowserCollector::poll_loop() {
    while (running_.load()) {
        process_history();

        // Sleep in small increments to respond quickly to stop()
        for (int i = 0; i < 50 && running_.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 5 seconds total
        }
    }
}

// Convert Unix seconds to Chrome timestamp (microseconds since Jan 1, 1601)
uint64_t BrowserCollector::get_chrome_timestamp(uint64_t unix_timestamp_sec) {
    // Note: 11644473600 is the number of seconds between Jan 1 1601 and Jan 1 1970
    return (unix_timestamp_sec + 11644473600ULL) * 1000000ULL;
}

// Convert Chrome timestamp to Unix seconds
uint64_t BrowserCollector::chrome_to_unix_timestamp(uint64_t chrome_timestamp_us) {
    return (chrome_timestamp_us / 1000000ULL) - 11644473600ULL;
}

void BrowserCollector::process_history() {
    if (!std::filesystem::exists(chrome_history_path_)) return;

    // 1. Copy the database to a temp file (Chrome locks the original)
    try {
        // Use copy_options::overwrite_existing
        std::filesystem::copy_file(chrome_history_path_, temp_history_path_, 
                                   std::filesystem::copy_options::overwrite_existing);
    } catch (const std::exception& e) {
        spdlog::debug("BrowserCollector failed to copy history: {}", e.what());
        return;
    }

    // 2. Open the copied SQLite database
    sqlite3* db = nullptr;
    if (sqlite3_open_v2(temp_history_path_.string().c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        spdlog::error("BrowserCollector failed to open temp DB");
        return;
    }

    // 3. Query new visits
    // We join 'visits' and 'urls' to get both the time, the url, and the page title
    const char* sql = R"(
        SELECT urls.url, urls.title, visits.visit_time 
        FROM visits 
        JOIN urls ON visits.url = urls.id 
        WHERE visits.visit_time > ? 
        ORDER BY visits.visit_time ASC
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("BrowserCollector failed to prepare query: {}", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    // Bind our last_visit_time_
    sqlite3_bind_int64(stmt, 1, last_visit_time_);

    // 4. Extract entries and emit events
    uint64_t max_time_seen = last_visit_time_;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* url_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* title_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        uint64_t visit_time_chrome = sqlite3_column_int64(stmt, 2);

        if (visit_time_chrome > max_time_seen) {
            max_time_seen = visit_time_chrome;
        }

        std::string url = url_text ? url_text : "";
        std::string title = title_text ? title_text : url;

        // Extract a clean "domain" or just use the title
        if (title.empty() || title == url) {
            title = "Visited: " + url; // Basic fallback
        }

        // Emit ActivityEvent
        ActivityEvent act_event;
        act_event.timestamp = chrome_to_unix_timestamp(visit_time_chrome);
        act_event.type = EventType::BrowserVisit;
        act_event.source = "chrome.exe"; // Generic but descriptive
        act_event.content = title;
        
        // Add URL as metadata so it's searchable!
        act_event.metadata["url"] = url;

        emit(std::move(act_event));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    // Attempt to remove the temp copy to stay clean
    try {
        std::filesystem::remove(temp_history_path_);
    } catch(...) {}

    // Update our watermark
    last_visit_time_ = max_time_seen;
}

}  // namespace mindtrace
