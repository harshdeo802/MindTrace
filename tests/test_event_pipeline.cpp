#include <gtest/gtest.h>
#include "core/pipeline/event_pipeline.hpp"
#include "core/storage/event_store.hpp"
#include "core/indexing/inverted_index.hpp"
#include <chrono>
#include <thread>

using namespace mindtrace;

TEST(TestEventPipeline, BasicProcessing) {
    auto store = std::make_shared<EventStore>(":memory:");
    auto pipeline = std::make_unique<EventPipeline>(1);

    pipeline->set_storage(store);

    // Add a stage that enriches metadata
    pipeline->add_stage([](ActivityEvent& event) {
        event.metadata["processed"] = "true";
    });

    pipeline->start();
    EXPECT_TRUE(pipeline->is_running());

    // Submit events
    ActivityEvent event;
    event.timestamp = 1000;
    event.type = EventType::FileOpened;
    event.source = "test.txt";
    event.content = "Test event";

    pipeline->submit(event);

    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    pipeline->stop();
    EXPECT_FALSE(pipeline->is_running());

    // Verify event was stored
    EXPECT_EQ(store->count(), 1u);
    auto stored = store->find_by_id(1);
    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->metadata.at("processed"), "true");
}

TEST(TestEventPipeline, MultipleStages) {
    auto store = std::make_shared<EventStore>(":memory:");
    auto pipeline = std::make_unique<EventPipeline>(1);

    pipeline->set_storage(store);

    // Stage 1: normalize content
    pipeline->add_stage([](ActivityEvent& event) {
        event.content = "[NORMALIZED] " + event.content;
    });

    // Stage 2: add metadata
    pipeline->add_stage([](ActivityEvent& event) {
        event.metadata["stage2"] = "done";
    });

    pipeline->start();

    ActivityEvent event;
    event.timestamp = 2000;
    event.type = EventType::FileModified;
    event.source = "main.cpp";
    event.content = "Modified main";

    pipeline->submit(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    pipeline->stop();

    auto stored = store->find_by_id(1);
    ASSERT_TRUE(stored.has_value());
    EXPECT_TRUE(stored->content.find("[NORMALIZED]") != std::string::npos);
    EXPECT_EQ(stored->metadata.at("stage2"), "done");
}

TEST(TestEventPipeline, ProcessedCount) {
    auto store = std::make_shared<EventStore>(":memory:");
    auto pipeline = std::make_unique<EventPipeline>(2);

    pipeline->set_storage(store);
    pipeline->start();

    for (int i = 0; i < 10; ++i) {
        ActivityEvent event;
        event.timestamp = static_cast<uint64_t>(i * 100);
        event.type = EventType::FileOpened;
        event.source = "file" + std::to_string(i) + ".txt";
        event.content = "Event " + std::to_string(i);
        pipeline->submit(event);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    pipeline->stop();

    EXPECT_EQ(pipeline->processed_count(), 10u);
    EXPECT_EQ(store->count(), 10u);
}

TEST(TestEventPipeline, StartStop) {
    auto pipeline = std::make_unique<EventPipeline>(1);

    EXPECT_FALSE(pipeline->is_running());

    pipeline->start();
    EXPECT_TRUE(pipeline->is_running());

    pipeline->stop();
    EXPECT_FALSE(pipeline->is_running());
}
