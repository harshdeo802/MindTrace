#include <gtest/gtest.h>
#include "core/event/activity_event.hpp"

using namespace mindtrace;

// ── Construction Tests ────────────────────────────────────────────────

TEST(TestActivityEvent, DefaultConstruction) {
    ActivityEvent event;
    EXPECT_EQ(event.id, 0);
    EXPECT_EQ(event.timestamp, 0);
    EXPECT_EQ(event.type, EventType::Unknown);
    EXPECT_TRUE(event.source.empty());
    EXPECT_TRUE(event.content.empty());
    EXPECT_TRUE(event.metadata.empty());
}

TEST(TestActivityEvent, FieldAssignment) {
    ActivityEvent event;
    event.id = 42;
    event.timestamp = 1700000000;
    event.type = EventType::FileOpened;
    event.source = "/home/user/document.txt";
    event.content = "Opened document.txt";
    event.metadata["size"] = "1024";

    EXPECT_EQ(event.id, 42);
    EXPECT_EQ(event.timestamp, 1700000000);
    EXPECT_EQ(event.type, EventType::FileOpened);
    EXPECT_EQ(event.source, "/home/user/document.txt");
    EXPECT_EQ(event.content, "Opened document.txt");
    EXPECT_EQ(event.metadata.at("size"), "1024");
}

// ── EventType Conversion Tests ────────────────────────────────────────

TEST(TestActivityEvent, EventTypeToString) {
    EXPECT_EQ(event_type_to_string(EventType::FileOpened), "FileOpened");
    EXPECT_EQ(event_type_to_string(EventType::FileModified), "FileModified");
    EXPECT_EQ(event_type_to_string(EventType::BrowserVisit), "BrowserVisit");
    EXPECT_EQ(event_type_to_string(EventType::WindowFocus), "WindowFocus");
    EXPECT_EQ(event_type_to_string(EventType::ClipboardCopy), "ClipboardCopy");
    EXPECT_EQ(event_type_to_string(EventType::ScreenshotCaptured), "ScreenshotCaptured");
    EXPECT_EQ(event_type_to_string(EventType::Unknown), "Unknown");
}

TEST(TestActivityEvent, StringToEventType) {
    EXPECT_EQ(string_to_event_type("FileOpened"), EventType::FileOpened);
    EXPECT_EQ(string_to_event_type("FileModified"), EventType::FileModified);
    EXPECT_EQ(string_to_event_type("BrowserVisit"), EventType::BrowserVisit);
    EXPECT_EQ(string_to_event_type("WindowFocus"), EventType::WindowFocus);
    EXPECT_EQ(string_to_event_type("ClipboardCopy"), EventType::ClipboardCopy);
    EXPECT_EQ(string_to_event_type("ScreenshotCaptured"), EventType::ScreenshotCaptured);
    EXPECT_EQ(string_to_event_type("invalid"), EventType::Unknown);
    EXPECT_EQ(string_to_event_type(""), EventType::Unknown);
}

// ── JSON Serialization Tests ──────────────────────────────────────────

TEST(TestActivityEvent, SerializationRoundTrip) {
    ActivityEvent original;
    original.id = 1;
    original.timestamp = 1700000000;
    original.type = EventType::FileModified;
    original.source = "C:/Users/test/main.cpp";
    original.content = "Edited main.cpp";
    original.metadata["extension"] = ".cpp";
    original.metadata["size"] = "2048";

    auto json = original.to_json();
    auto deserialized = ActivityEvent::from_json(json);

    EXPECT_EQ(deserialized.id, original.id);
    EXPECT_EQ(deserialized.timestamp, original.timestamp);
    EXPECT_EQ(deserialized.type, original.type);
    EXPECT_EQ(deserialized.source, original.source);
    EXPECT_EQ(deserialized.content, original.content);
    EXPECT_EQ(deserialized.metadata, original.metadata);
}

TEST(TestActivityEvent, SerializationEmptyMetadata) {
    ActivityEvent event;
    event.timestamp = 100;
    event.type = EventType::WindowFocus;
    event.source = "VSCode";
    event.content = "Window focused";

    auto json = event.to_json();
    auto restored = ActivityEvent::from_json(json);

    EXPECT_EQ(restored.type, EventType::WindowFocus);
    EXPECT_TRUE(restored.metadata.empty());
}

TEST(TestActivityEvent, JsonContainsExpectedFields) {
    ActivityEvent event;
    event.id = 5;
    event.timestamp = 999;
    event.type = EventType::BrowserVisit;
    event.source = "https://example.com";
    event.content = "Visited example";

    auto json = event.to_json();

    EXPECT_TRUE(json.contains("id"));
    EXPECT_TRUE(json.contains("timestamp"));
    EXPECT_TRUE(json.contains("type"));
    EXPECT_TRUE(json.contains("source"));
    EXPECT_TRUE(json.contains("content"));
    EXPECT_TRUE(json.contains("metadata"));
    EXPECT_EQ(json["type"], "BrowserVisit");
}

// ── Keyword Extraction Tests ──────────────────────────────────────────

TEST(TestActivityEvent, ExtractKeywords) {
    ActivityEvent event;
    event.type = EventType::FileModified;
    event.source = "main.cpp";
    event.content = "Edited the main source file";

    auto keywords = extract_keywords(event);

    EXPECT_FALSE(keywords.empty());
    // Should contain "main", "edited", "source", "file", "filemodified"
    EXPECT_TRUE(std::find(keywords.begin(), keywords.end(), "main") != keywords.end());
    EXPECT_TRUE(std::find(keywords.begin(), keywords.end(), "edited") != keywords.end());
    EXPECT_TRUE(std::find(keywords.begin(), keywords.end(), "filemodified") != keywords.end());
}

TEST(TestActivityEvent, ExtractKeywordsDeduplicated) {
    ActivityEvent event;
    event.type = EventType::FileOpened;
    event.source = "test.txt";
    event.content = "test test test";

    auto keywords = extract_keywords(event);

    // Count occurrences of "test"
    auto count = std::count(keywords.begin(), keywords.end(), "test");
    EXPECT_EQ(count, 1);  // Should be deduplicated
}

// ── Equality Tests ────────────────────────────────────────────────────

TEST(TestActivityEvent, EqualityOperator) {
    ActivityEvent a, b;
    a.id = 1; a.timestamp = 100; a.type = EventType::FileOpened;
    a.source = "test"; a.content = "content";

    b = a;
    EXPECT_EQ(a, b);

    b.timestamp = 200;
    EXPECT_NE(a, b);
}
