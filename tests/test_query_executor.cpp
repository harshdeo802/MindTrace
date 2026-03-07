#include <gtest/gtest.h>
#include "search/query_executor.hpp"
#include "core/storage/event_store.hpp"
#include "core/indexing/inverted_index.hpp"

using namespace mindtrace;

class QueryExecutorTest : public ::testing::Test {
protected:
    void SetUp() override {
        store = std::make_shared<EventStore>(":memory:");
        index = std::make_shared<InvertedIndex>();
        executor = std::make_unique<QueryExecutor>(store, index);

        // Seed test data
        auto save_and_index = [&](uint64_t ts, EventType type,
                                    const std::string& src, const std::string& content) {
            ActivityEvent e;
            e.timestamp = ts;
            e.type = type;
            e.source = src;
            e.content = content;
            uint64_t id = store->save(e);
            e.id = id;
            index->add(e);
        };

        // Today's timestamp (roughly)
        uint64_t now = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
        uint64_t today_start = (now / 86400) * 86400;

        save_and_index(today_start + 3600 * 9, EventType::FileOpened,
            "main.cpp", "Opened main source file");
        save_and_index(today_start + 3600 * 10, EventType::FileModified,
            "main.cpp", "Edited main source file");
        save_and_index(today_start + 3600 * 11, EventType::BrowserVisit,
            "stackoverflow.com", "Searched for C++ templates");
        save_and_index(today_start + 3600 * 14, EventType::FileModified,
            "test.cpp", "Wrote unit tests for module");

        // Yesterday
        save_and_index(today_start - 3600 * 10, EventType::FileOpened,
            "readme.md", "Opened project readme");
    }

    std::shared_ptr<EventStore> store;
    std::shared_ptr<InvertedIndex> index;
    std::unique_ptr<QueryExecutor> executor;
};

TEST_F(QueryExecutorTest, SearchByKeyword) {
    auto results = executor->search("main source");
    EXPECT_GE(results.size(), 1u);
}

TEST_F(QueryExecutorTest, SearchFilesEditedToday) {
    auto results = executor->search("files edited today");
    // Should find the FileModified events from today
    for (const auto& r : results) {
        EXPECT_EQ(r.type, EventType::FileModified);
    }
}

TEST_F(QueryExecutorTest, SearchBrowserVisits) {
    auto results = executor->search("websites visited today");
    EXPECT_GE(results.size(), 1u);
    for (const auto& r : results) {
        EXPECT_EQ(r.type, EventType::BrowserVisit);
    }
}

TEST_F(QueryExecutorTest, EmptyQuery) {
    auto results = executor->search("");
    // Should return recent events (up to 100)
    EXPECT_LE(results.size(), 100u);
}

TEST_F(QueryExecutorTest, ExecutePlanDirectly) {
    QueryPlan plan;
    plan.use_keyword_index = true;
    plan.keywords = {"main"};

    auto results = executor->execute(plan);
    EXPECT_GE(results.size(), 1u);
}
