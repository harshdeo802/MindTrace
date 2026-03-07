#ifndef MINDTRACE_COLLECTORS_BROWSER_COLLECTOR_HPP
#define MINDTRACE_COLLECTORS_BROWSER_COLLECTOR_HPP

#include "collectors/collector_base.hpp"

#include <thread>
#include <chrono>
#include <filesystem>
#include <sqlite3.h>

namespace mindtrace {

/// Collector for browser history (Google Chrome).
/// Polls Chrome's History SQLite database.
class BrowserCollector : public CollectorBase {
public:
    BrowserCollector();
    ~BrowserCollector() override;

    void start() override;
    void stop() override;

private:
    void poll_loop();
    void process_history();
    uint64_t get_chrome_timestamp(uint64_t unix_timestamp_sec);
    uint64_t chrome_to_unix_timestamp(uint64_t chrome_timestamp_us);

    std::thread poll_thread_;
    std::chrono::seconds poll_interval_{5};
    uint64_t last_visit_time_ = 0;
    
    std::filesystem::path chrome_history_path_;
    std::filesystem::path temp_history_path_;
};

}  // namespace mindtrace

#endif  // MINDTRACE_COLLECTORS_BROWSER_COLLECTOR_HPP
