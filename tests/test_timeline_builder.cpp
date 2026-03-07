#include <gtest/gtest.h>
#include "timeline/timeline_builder.hpp"

using namespace mindtrace;

class TimelineBuilderTest : public ::testing::Test {
protected:
    ActivityEvent make_event(uint64_t id, uint64_t ts,
                             const std::string& source, const std::string& content) {
        ActivityEvent e;
        e.id = id;
        e.timestamp = ts;
        e.type = EventType::FileModified;
        e.source = source;
        e.content = content;
        return e;
    }
};

TEST_F(TimelineBuilderTest, EmptyEvents) {
    TimelineBuilder builder;
    auto timeline = builder.build({});
    EXPECT_EQ(timeline.sessions.size(), 0u);
    EXPECT_EQ(timeline.total_events(), 0u);
}

TEST_F(TimelineBuilderTest, SingleEvent) {
    TimelineBuilder builder;
    std::vector<ActivityEvent> events = {
        make_event(1, 1000, "main.cpp", "Edited")
    };

    auto timeline = builder.build(events);
    EXPECT_EQ(timeline.sessions.size(), 1u);
    EXPECT_EQ(timeline.sessions[0].events.size(), 1u);
    EXPECT_EQ(timeline.total_events(), 1u);
}

TEST_F(TimelineBuilderTest, EventsGroupedByTimeProximity) {
    TimelineBuilder builder(600);  // 10 minute gap threshold

    std::vector<ActivityEvent> events = {
        make_event(1, 1000, "main.cpp", "Edit 1"),
        make_event(2, 1300, "main.cpp", "Edit 2"),    // 5 min later
        make_event(3, 1500, "test.cpp", "Edit 3"),    // 3.3 min later
        make_event(4, 3000, "readme.md", "Edit 4"),   // 25 min later — new session
        make_event(5, 3200, "readme.md", "Edit 5"),   // 3.3 min later
    };

    auto timeline = builder.build(events);
    EXPECT_EQ(timeline.sessions.size(), 2u);
    EXPECT_EQ(timeline.sessions[0].events.size(), 3u);
    EXPECT_EQ(timeline.sessions[1].events.size(), 2u);
}

TEST_F(TimelineBuilderTest, SessionDuration) {
    TimelineBuilder builder(600);

    std::vector<ActivityEvent> events = {
        make_event(1, 1000, "a.cpp", "Start"),
        make_event(2, 1500, "a.cpp", "End"),
    };

    auto timeline = builder.build(events);
    EXPECT_EQ(timeline.sessions.size(), 1u);
    EXPECT_EQ(timeline.sessions[0].duration(), 500u);
}

TEST_F(TimelineBuilderTest, PrimaryAppDetection) {
    TimelineBuilder builder(3600);

    std::vector<ActivityEvent> events = {
        make_event(1, 1000, "vscode.exe", "Edit 1"),
        make_event(2, 1100, "vscode.exe", "Edit 2"),
        make_event(3, 1200, "chrome.exe", "Browse"),
        make_event(4, 1300, "vscode.exe", "Edit 3"),
    };

    auto timeline = builder.build(events);
    EXPECT_EQ(timeline.sessions.size(), 1u);
    EXPECT_EQ(timeline.sessions[0].primary_app, "vscode.exe");
}

TEST_F(TimelineBuilderTest, TimeRangeFilter) {
    TimelineBuilder builder(600);

    std::vector<ActivityEvent> events = {
        make_event(1, 1000, "a.cpp", "Before"),
        make_event(2, 2000, "b.cpp", "During"),
        make_event(3, 3000, "c.cpp", "After"),
    };

    auto timeline = builder.build(events, 1500, 2500);
    EXPECT_EQ(timeline.total_events(), 1u);
    EXPECT_EQ(timeline.period_start, 1500u);
    EXPECT_EQ(timeline.period_end, 2500u);
}

TEST_F(TimelineBuilderTest, UnsortedEventsHandled) {
    TimelineBuilder builder(600);

    // Events in reverse order
    std::vector<ActivityEvent> events = {
        make_event(3, 3000, "c.cpp", "Third"),
        make_event(1, 1000, "a.cpp", "First"),
        make_event(2, 2000, "b.cpp", "Second"),
    };

    auto timeline = builder.build(events);
    // Should still group correctly after sorting
    EXPECT_GE(timeline.sessions.size(), 1u);
    EXPECT_EQ(timeline.total_events(), 3u);
}

TEST_F(TimelineBuilderTest, CustomGapThreshold) {
    // Very small gap threshold
    TimelineBuilder builder(1);

    std::vector<ActivityEvent> events = {
        make_event(1, 100, "a.cpp", "A"),
        make_event(2, 105, "b.cpp", "B"),  // 5s gap > 1s threshold
    };

    auto timeline = builder.build(events);
    EXPECT_EQ(timeline.sessions.size(), 2u);
}
