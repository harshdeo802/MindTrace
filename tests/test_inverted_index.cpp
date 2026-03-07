#include <gtest/gtest.h>
#include "core/indexing/inverted_index.hpp"

using namespace mindtrace;

class InvertedIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        index = std::make_unique<InvertedIndex>();
    }

    ActivityEvent make_event(uint64_t id, uint64_t ts, EventType type,
                             const std::string& source, const std::string& content) {
        ActivityEvent e;
        e.id = id;
        e.timestamp = ts;
        e.type = type;
        e.source = source;
        e.content = content;
        return e;
    }

    std::unique_ptr<InvertedIndex> index;
};

TEST_F(InvertedIndexTest, AddAndSearchKeyword) {
    index->add(make_event(1, 1000, EventType::FileModified, "main.cpp", "Edited main source file"));
    index->add(make_event(2, 2000, EventType::FileOpened, "test.cpp", "Opened test file"));

    auto results = index->search_keyword("main");
    EXPECT_TRUE(results.count(1));

    auto results2 = index->search_keyword("test");
    EXPECT_TRUE(results2.count(2));
}

TEST_F(InvertedIndexTest, SearchTimeRange) {
    index->add(make_event(1, 3600 * 10, EventType::FileOpened, "a.txt", "Morning work"));
    index->add(make_event(2, 3600 * 14, EventType::FileModified, "b.txt", "Afternoon work"));
    index->add(make_event(3, 3600 * 20, EventType::BrowserVisit, "url", "Evening browsing"));

    // Search during afternoon (hour 13-15)
    auto results = index->search_time_range(3600 * 13, 3600 * 15);
    EXPECT_EQ(results.size(), 1u);
    EXPECT_TRUE(results.count(2));

    // Search whole day
    auto all = index->search_time_range(0, 3600 * 24);
    EXPECT_EQ(all.size(), 3u);
}

TEST_F(InvertedIndexTest, SearchApplication) {
    index->add(make_event(1, 1000, EventType::FileModified,
        "C:/Projects/main.cpp", "Editing source"));
    index->add(make_event(2, 2000, EventType::FileModified,
        "C:/Projects/test.cpp", "Writing tests"));
    index->add(make_event(3, 3000, EventType::BrowserVisit,
        "chrome.exe", "Browsing"));

    auto results = index->search_application("chrome");
    EXPECT_EQ(results.size(), 1u);
    EXPECT_TRUE(results.count(3));
}

TEST_F(InvertedIndexTest, EventCount) {
    EXPECT_EQ(index->event_count(), 0u);

    index->add(make_event(1, 1000, EventType::FileOpened, "a.txt", "A"));
    index->add(make_event(2, 2000, EventType::FileModified, "b.txt", "B"));

    EXPECT_EQ(index->event_count(), 2u);
}

TEST_F(InvertedIndexTest, KeywordCount) {
    EXPECT_EQ(index->keyword_count(), 0u);

    index->add(make_event(1, 1000, EventType::FileOpened, "a.txt", "Hello world"));
    EXPECT_GT(index->keyword_count(), 0u);
}

TEST_F(InvertedIndexTest, ClearIndex) {
    index->add(make_event(1, 1000, EventType::FileOpened, "a.txt", "Test"));
    EXPECT_GT(index->event_count(), 0u);

    index->clear();
    EXPECT_EQ(index->event_count(), 0u);
    EXPECT_EQ(index->keyword_count(), 0u);
}

TEST_F(InvertedIndexTest, CaseInsensitiveSearch) {
    index->add(make_event(1, 1000, EventType::FileModified, "Main.cpp", "Edited FILE"));

    auto results = index->search_keyword("MAIN");
    EXPECT_TRUE(results.count(1));

    auto results2 = index->search_keyword("file");
    EXPECT_TRUE(results2.count(1));
}
