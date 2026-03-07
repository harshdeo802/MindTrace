#include <gtest/gtest.h>
#include "search/query_parser.hpp"

using namespace mindtrace;

TEST(TestQueryParser, ParseTodayQuery) {
    QueryParser parser;
    auto spec = parser.parse("What was I doing today");

    EXPECT_TRUE(spec.time_start.has_value());
    EXPECT_TRUE(spec.time_end.has_value());
    EXPECT_GT(*spec.time_end, *spec.time_start);
}

TEST(TestQueryParser, ParseYesterdayQuery) {
    QueryParser parser;
    auto spec = parser.parse("files edited yesterday");

    EXPECT_TRUE(spec.time_start.has_value());
    EXPECT_TRUE(spec.time_end.has_value());
    EXPECT_EQ(spec.event_type, EventType::FileModified);
}

TEST(TestQueryParser, ParseFileEditedQuery) {
    QueryParser parser;
    auto spec = parser.parse("files edited today");

    EXPECT_EQ(spec.event_type, EventType::FileModified);
    EXPECT_TRUE(spec.time_start.has_value());
}

TEST(TestQueryParser, ParseBrowserQuery) {
    QueryParser parser;
    auto spec = parser.parse("websites visited this week");

    EXPECT_EQ(spec.event_type, EventType::BrowserVisit);
    EXPECT_TRUE(spec.time_start.has_value());
}

TEST(TestQueryParser, ParseWindowFocusQuery) {
    QueryParser parser;
    auto spec = parser.parse("window focus events");

    EXPECT_EQ(spec.event_type, EventType::WindowFocus);
}

TEST(TestQueryParser, ParseClipboardQuery) {
    QueryParser parser;
    auto spec = parser.parse("clipboard copy history");

    EXPECT_EQ(spec.event_type, EventType::ClipboardCopy);
}

TEST(TestQueryParser, ExtractKeywords) {
    QueryParser parser;
    auto spec = parser.parse("main.cpp project build");

    // "main" and "project" and "build" should be in keywords
    // (stop words removed)
    EXPECT_FALSE(spec.keywords.empty());
}

TEST(TestQueryParser, RawQueryPreserved) {
    QueryParser parser;
    auto spec = parser.parse("test query string");

    EXPECT_EQ(spec.raw_query, "test query string");
}

TEST(TestQueryParser, LastWeekQuery) {
    QueryParser parser;
    auto spec = parser.parse("What did I do last week");

    EXPECT_TRUE(spec.time_start.has_value());
    EXPECT_TRUE(spec.time_end.has_value());
    EXPECT_GT(*spec.time_end, *spec.time_start);
    // Last week should be ~7 days before this week
    uint64_t diff = *spec.time_end - *spec.time_start;
    EXPECT_GE(diff, 6 * 86400);  // At least 6 days
    EXPECT_LE(diff, 7 * 86400);  // At most 7 days
}

TEST(TestQueryParser, EmptyQuery) {
    QueryParser parser;
    auto spec = parser.parse("");

    EXPECT_FALSE(spec.time_start.has_value());
    EXPECT_FALSE(spec.event_type.has_value());
    EXPECT_TRUE(spec.keywords.empty());
}
