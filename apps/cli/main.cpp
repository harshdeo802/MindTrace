#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>

#include "core/event/activity_event.hpp"
#include "core/storage/event_store.hpp"
#include "core/indexing/inverted_index.hpp"
#include "search/query_executor.hpp"
#include "timeline/timeline_builder.hpp"
#include "collectors/browser_collector.hpp"
#include "collectors/window_focus_collector.hpp"
#include "collectors/filesystem_collector.hpp"
#include "common/utils.hpp"
#include "common/config.hpp"

#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

#include <spdlog/spdlog.h>

using namespace mindtrace;

std::atomic<bool> daemon_running{true};

void signal_handler(int signum) {
    if (signum == SIGINT) {
        std::cout << "\n[!] Caught interrupt signal, stopping mindtrace daemon...\n";
        daemon_running.store(false);
    }
}

void print_header() {
    std::cout << R"(
  __  __ _           _ _____
 |  \/  (_)         | |_   _|
 | \  / |_ _ __   __| | | |  _ __ __ _  ___ ___
 | |\/| | | '_ \ / _` | | | | '__/ _` |/ __/ _ \
 | |  | | | | | | (_| | | | | | | (_| | (_|  __/
 |_|  |_|_|_| |_|\__,_| |_| |_|  \__,_|\___\___|

  Digital Memory Search Engine v1.0.0
)" << std::endl;
}

void print_usage() {
    std::cout << "Usage:\n"
              << "  mindtrace search <query>     Search your activity history\n"
              << "  mindtrace timeline [day]     Show activity timeline\n"
              << "  mindtrace stats              Show database statistics\n"
              << "  mindtrace daemon             Run in background and collect events\n"
              << "  mindtrace demo               Load demo data and run sample queries\n"
              << "  mindtrace help               Show this help message\n"
              << std::endl;
}

void print_event(const ActivityEvent& event) {
    std::cout << "  " << format_time_only(event.timestamp)
              << "  [" << event_type_to_string(event.type) << "]  "
              << truncate(event.source, 40) << "  -  "
              << truncate(event.content, 50) << "\n";
}

void cmd_search(const std::string& query,
                std::shared_ptr<EventStore> store,
                std::shared_ptr<InvertedIndex> index) {
    QueryExecutor executor(store, index);
    auto results = executor.search(query);

    std::cout << "\n🔍 Search: \"" << query << "\"\n";
    std::cout << "   Found " << results.size() << " result(s)\n\n";

    if (results.empty()) {
        std::cout << "   No results found.\n";
        return;
    }

    for (const auto& event : results) {
        print_event(event);
    }
    std::cout << std::endl;
}

void cmd_timeline(const std::string& day,
                  std::shared_ptr<EventStore> store) {
    TimelineBuilder builder;

    uint64_t now = current_timestamp();
    uint64_t today_start = (now / 86400) * 86400;
    uint64_t period_start, period_end;

    if (day == "yesterday") {
        period_start = today_start - 86400;
        period_end = today_start - 1;
    } else {
        period_start = today_start;
        period_end = now;
    }

    auto events = store->find_by_time_range(period_start, period_end);
    auto timeline = builder.build(events, period_start, period_end);

    std::cout << "\n📅 Timeline: " << (day == "yesterday" ? "Yesterday" : "Today") << "\n";
    std::cout << "   " << format_timestamp(period_start) << " - "
              << format_timestamp(period_end) << "\n";
    std::cout << "   " << timeline.total_events() << " event(s) in "
              << timeline.sessions.size() << " session(s)\n\n";

    for (size_t i = 0; i < timeline.sessions.size(); ++i) {
        const auto& session = timeline.sessions[i];
        std::cout << "  --- Session " << (i + 1) << " ("
                  << format_time_only(session.start_time) << " - "
                  << format_time_only(session.end_time) << ") "
                  << "[" << session.primary_app << "] ---\n";

        for (const auto& event : session.events) {
            print_event(event);
        }
        std::cout << "\n";
    }
}

void cmd_stats(std::shared_ptr<EventStore> store,
               std::shared_ptr<InvertedIndex> index) {
    std::cout << "\n📊 MindTrace Statistics\n";
    std::cout << "   Total events:     " << store->count() << "\n";
    std::cout << "   Indexed events:   " << index->event_count() << "\n";
    std::cout << "   Unique keywords:  " << index->keyword_count() << "\n";
    std::cout << std::endl;
}

void cmd_daemon(std::shared_ptr<EventStore> store,
                std::shared_ptr<InvertedIndex> index) {
    std::cout << "\n🚀 Starting MindTrace Background Daemon...\n";
    
    // Setup signal handler for graceful shutdown
    std::signal(SIGINT, signal_handler);

    // Identify Documents path
#ifdef _WIN32
    char path[MAX_PATH];
    std::filesystem::path docs_path = std::filesystem::current_path(); // fallback
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, path))) {
        docs_path = path;
    }
#else
    std::filesystem::path docs_path = std::getenv("HOME");
    docs_path /= "Documents";
#endif

    std::cout << "   Watching directory: " << docs_path.string() << "\n\n";

    // Initialize Collectors
    mindtrace::WindowFocusCollector window_collector;
    mindtrace::BrowserCollector browser_collector;
    mindtrace::FileSystemCollector fs_collector(docs_path, std::chrono::seconds(2));

    // Shared callback
    auto callback = [&](mindtrace::ActivityEvent e) {
        uint64_t id = store->save(e);
        e.id = id;
        index->add(e);
        spdlog::info("Captured: [{}] {}", event_type_to_string(e.type), e.content);
        // Print to console for visibility
        std::cout << "  " << format_time_only(e.timestamp) 
                  << "  [" << event_type_to_string(e.type) << "]  " 
                  << truncate(e.content, 60) << "\n";
    };

    window_collector.set_callback(callback);
    browser_collector.set_callback(callback);
    fs_collector.set_callback(callback);

    // Start all
    window_collector.start();
    browser_collector.start();
    fs_collector.start();

    std::cout << "   (Daemon running. Press Ctrl+C to stop tracking)\n\n";

    // Block until signaled
    while (daemon_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Stop all
    std::cout << "\n   Shutting down collectors gracefully...\n";
    window_collector.stop();
    browser_collector.stop();
    fs_collector.stop();
    std::cout << "   Daemon stopped.\n\n";
}

void cmd_demo(std::shared_ptr<EventStore> store,
              std::shared_ptr<InvertedIndex> index) {
    std::cout << "\n🎮 Loading demo data...\n\n";

    uint64_t now = current_timestamp();
    uint64_t today_start = (now / 86400) * 86400;

    std::vector<ActivityEvent> demo_events = {
        {0, today_start + 9 * 3600,       EventType::WindowFocus,  "VSCode",           "VSCode opened",                {}},
        {0, today_start + 9 * 3600 + 300, EventType::FileOpened,   "main.cpp",         "Opened main.cpp",             {{"extension", ".cpp"}}},
        {0, today_start + 9 * 3600 + 600, EventType::FileModified, "main.cpp",         "Edited main.cpp",             {{"extension", ".cpp"}}},
        {0, today_start + 10 * 3600,      EventType::BrowserVisit, "stackoverflow.com","Searched C++ template help",  {}},
        {0, today_start + 10 * 3600 + 300,EventType::ClipboardCopy,"stackoverflow.com","Copied code snippet",         {}},
        {0, today_start + 11 * 3600,      EventType::FileModified, "utils.hpp",        "Updated utility functions",   {{"extension", ".hpp"}}},
        {0, today_start + 12 * 3600,      EventType::WindowFocus,  "Terminal",          "Switched to terminal",        {}},
        {0, today_start + 12 * 3600 + 60, EventType::FileModified, "CMakeLists.txt",   "Updated build config",        {}},
        {0, today_start + 14 * 3600,      EventType::BrowserVisit, "github.com",       "Reviewed pull request",       {}},
        {0, today_start + 15 * 3600,      EventType::FileModified, "test_main.cpp",    "Wrote unit tests",            {{"extension", ".cpp"}}},
    };

    for (auto& event : demo_events) {
        uint64_t id = store->save(event);
        event.id = id;
        index->add(event);
    }
    
    std::cout << "   Loaded " << demo_events.size() << " demo events.\n\n";



    // Run sample queries
    std::cout << "------------------------------------------------\n";
    cmd_search("files edited today", store, index);

    std::cout << "------------------------------------------------\n";
    cmd_search("websites visited today", store, index);

    std::cout << "------------------------------------------------\n";
    cmd_timeline("today", store);

    std::cout << "------------------------------------------------\n";
    cmd_stats(store, index);
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    spdlog::set_level(spdlog::level::warn);

    print_header();

    if (argc < 2) {
        print_usage();
        return 0;
    }

    std::string command = argv[1];

    // Initialize storage
    std::string db_path = Config::default_db_path();
    auto store = std::make_shared<EventStore>(db_path);
    auto index = std::make_shared<InvertedIndex>();

    // Rebuild index from stored events
    auto all_events = store->find_by_time_range(0, current_timestamp());
    for (const auto& event : all_events) {
        index->add(event);
    }

    if (command == "search" && argc >= 3) {
        // Join remaining args as query
        std::string query;
        for (int i = 2; i < argc; ++i) {
            if (i > 2) query += " ";
            query += argv[i];
        }
        cmd_search(query, store, index);

    } else if (command == "timeline") {
        std::string day = (argc >= 3) ? argv[2] : "today";
        cmd_timeline(day, store);

    } else if (command == "stats") {
        cmd_stats(store, index);

    } else if (command == "daemon") {
        cmd_daemon(store, index);

    } else if (command == "demo") {
        cmd_demo(store, index);

    } else if (command == "help") {
        print_usage();

    } else {
        std::cerr << "Unknown command: " << command << "\n";
        print_usage();
        return 1;
    }

    return 0;
}
