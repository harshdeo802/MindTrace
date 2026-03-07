#include <gtest/gtest.h>
#include "collectors/collector_base.hpp"
#include "collectors/filesystem_collector.hpp"
#include "collectors/browser_collector.hpp"
#include "collectors/window_focus_collector.hpp"
#include "collectors/clipboard_collector.hpp"
#include <filesystem>
#include <fstream>
#include <thread>
#include <atomic>
#include <vector>

using namespace mindtrace;

// ── CollectorBase Tests ───────────────────────────────────────────────

class TestCollector : public CollectorBase {
public:
    TestCollector() : CollectorBase("TestCollector") {}

    void start() override { running_ = true; }
    void stop() override { running_ = false; }

    // Expose emit for testing
    void emit_test_event(ActivityEvent event) {
        emit(std::move(event));
    }
};

TEST(TestCollectors, CallbackMechanism) {
    TestCollector collector;
    std::vector<ActivityEvent> received;

    collector.set_callback([&](ActivityEvent event) {
        received.push_back(std::move(event));
    });

    ActivityEvent event;
    event.timestamp = 1000;
    event.type = EventType::FileOpened;
    event.source = "test.txt";
    event.content = "Test";

    collector.emit_test_event(event);

    EXPECT_EQ(received.size(), 1u);
    EXPECT_EQ(received[0].type, EventType::FileOpened);
}

TEST(TestCollectors, CollectorName) {
    TestCollector collector;
    EXPECT_EQ(collector.name(), "TestCollector");

    BrowserCollector browser;
    EXPECT_EQ(browser.name(), "BrowserCollector");

    WindowFocusCollector window;
    EXPECT_EQ(window.name(), "WindowFocusCollector");

    ClipboardCollector clipboard;
    EXPECT_EQ(clipboard.name(), "ClipboardCollector");
}

TEST(TestCollectors, StartStop) {
    TestCollector collector;

    EXPECT_FALSE(collector.is_running());
    collector.start();
    EXPECT_TRUE(collector.is_running());
    collector.stop();
    EXPECT_FALSE(collector.is_running());
}

TEST(TestCollectors, NoCallbackDoesNotCrash) {
    TestCollector collector;
    // No callback set — should not crash
    ActivityEvent event;
    event.type = EventType::FileOpened;
    collector.emit_test_event(event);
}

// ── Stub Collectors Start/Stop ───────────────────────────────────────

TEST(TestCollectors, BrowserCollectorStartStop) {
    BrowserCollector collector;
    EXPECT_FALSE(collector.is_running());
    collector.start();
    EXPECT_TRUE(collector.is_running());
    collector.stop();
    EXPECT_FALSE(collector.is_running());
}

TEST(TestCollectors, WindowFocusCollectorStartStop) {
    WindowFocusCollector collector;
    collector.start();
    EXPECT_TRUE(collector.is_running());
    collector.stop();
    EXPECT_FALSE(collector.is_running());
}

TEST(TestCollectors, ClipboardCollectorStartStop) {
    ClipboardCollector collector;
    collector.start();
    EXPECT_TRUE(collector.is_running());
    collector.stop();
    EXPECT_FALSE(collector.is_running());
}

// ── FileSystemCollector Tests ────────────────────────────────────────

TEST(TestCollectors, FileSystemCollectorDetectsFiles) {
    // Create a temporary directory with a test file
    auto temp_dir = std::filesystem::temp_directory_path() / "mindtrace_test";
    std::filesystem::create_directories(temp_dir);

    // Create a test file
    {
        std::ofstream f(temp_dir / "test_file.txt");
        f << "Hello, MindTrace!";
    }

    std::vector<ActivityEvent> events;
    FileSystemCollector collector(temp_dir, std::chrono::seconds(1));
    collector.set_callback([&](ActivityEvent event) {
        events.push_back(std::move(event));
    });

    collector.start();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    collector.stop();

    // Should have discovered at least one file
    EXPECT_GE(events.size(), 1u);
    EXPECT_EQ(events[0].type, EventType::FileOpened);

    // Cleanup
    std::filesystem::remove_all(temp_dir);
}
