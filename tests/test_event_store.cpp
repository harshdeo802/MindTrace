#include <gtest/gtest.h>
#include "core/storage/event_store.hpp"

using namespace mindtrace;

class EventStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        store = std::make_unique<EventStore>(":memory:");
    }

    ActivityEvent make_event(uint64_t ts, EventType type,
                             const std::string& source,
                             const std::string& content) {
        ActivityEvent e;
        e.timestamp = ts;
        e.type = type;
        e.source = source;
        e.content = content;
        return e;
    }

    std::unique_ptr<EventStore> store;
};

// ── CRUD Tests ────────────────────────────────────────────────────────

TEST_F(EventStoreTest, SaveAndRetrieve) {
    auto event = make_event(1000, EventType::FileOpened, "test.txt", "Opened file");
    event.metadata["key"] = "value";

    uint64_t id = store->save(event);
    EXPECT_GT(id, 0u);

    auto retrieved = store->find_by_id(id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->timestamp, 1000u);
    EXPECT_EQ(retrieved->type, EventType::FileOpened);
    EXPECT_EQ(retrieved->source, "test.txt");
    EXPECT_EQ(retrieved->content, "Opened file");
    EXPECT_EQ(retrieved->metadata.at("key"), "value");
}

TEST_F(EventStoreTest, FindByIdNotFound) {
    auto result = store->find_by_id(99999);
    EXPECT_FALSE(result.has_value());
}

TEST_F(EventStoreTest, Count) {
    EXPECT_EQ(store->count(), 0u);

    store->save(make_event(100, EventType::FileOpened, "a.txt", "A"));
    store->save(make_event(200, EventType::FileModified, "b.txt", "B"));
    store->save(make_event(300, EventType::BrowserVisit, "url", "C"));

    EXPECT_EQ(store->count(), 3u);
}

TEST_F(EventStoreTest, Clear) {
    store->save(make_event(100, EventType::FileOpened, "a.txt", "A"));
    store->save(make_event(200, EventType::FileModified, "b.txt", "B"));
    EXPECT_EQ(store->count(), 2u);

    store->clear();
    EXPECT_EQ(store->count(), 0u);
}

// ── Batch Insert ──────────────────────────────────────────────────────

TEST_F(EventStoreTest, SaveBatch) {
    std::vector<ActivityEvent> events;
    for (int i = 0; i < 100; ++i) {
        events.push_back(make_event(
            static_cast<uint64_t>(i * 100),
            EventType::FileModified,
            "file" + std::to_string(i) + ".txt",
            "Batch event " + std::to_string(i)));
    }

    store->save_batch(events);
    EXPECT_EQ(store->count(), 100u);
}

// ── Query Tests ───────────────────────────────────────────────────────

TEST_F(EventStoreTest, FindByTimeRange) {
    store->save(make_event(100, EventType::FileOpened, "a.txt", "Event A"));
    store->save(make_event(200, EventType::FileModified, "b.txt", "Event B"));
    store->save(make_event(300, EventType::BrowserVisit, "url", "Event C"));
    store->save(make_event(400, EventType::WindowFocus, "app", "Event D"));

    auto results = store->find_by_time_range(150, 350);
    EXPECT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].timestamp, 200u);
    EXPECT_EQ(results[1].timestamp, 300u);
}

TEST_F(EventStoreTest, FindByType) {
    store->save(make_event(100, EventType::FileOpened, "a.txt", "A"));
    store->save(make_event(200, EventType::FileModified, "b.txt", "B"));
    store->save(make_event(300, EventType::FileOpened, "c.txt", "C"));
    store->save(make_event(400, EventType::BrowserVisit, "url", "D"));

    auto results = store->find_by_type(EventType::FileOpened);
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(EventStoreTest, FindByKeyword) {
    store->save(make_event(100, EventType::FileModified, "main.cpp", "Edited main source"));
    store->save(make_event(200, EventType::FileModified, "test.cpp", "Wrote unit tests"));
    store->save(make_event(300, EventType::BrowserVisit, "google.com", "Searched google"));

    auto results = store->find_by_keyword("main");
    EXPECT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].source, "main.cpp");

    // Search in content
    auto results2 = store->find_by_keyword("unit tests");
    EXPECT_EQ(results2.size(), 1u);

    // Search across both fields
    auto results3 = store->find_by_keyword("google");
    EXPECT_EQ(results3.size(), 1u);
}

// ── Metadata Persistence ──────────────────────────────────────────────

TEST_F(EventStoreTest, MetadataPersistence) {
    auto event = make_event(100, EventType::FileOpened, "doc.pdf", "Opened PDF");
    event.metadata["pages"] = "42";
    event.metadata["author"] = "John";

    uint64_t id = store->save(event);
    auto retrieved = store->find_by_id(id);

    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->metadata.size(), 2u);
    EXPECT_EQ(retrieved->metadata.at("pages"), "42");
    EXPECT_EQ(retrieved->metadata.at("author"), "John");
}

// ── Move Semantics ────────────────────────────────────────────────────

TEST_F(EventStoreTest, MoveConstruction) {
    store->save(make_event(100, EventType::FileOpened, "a.txt", "A"));

    EventStore moved(std::move(*store));
    EXPECT_EQ(moved.count(), 1u);
}
