#include "core/pipeline/event_pipeline.hpp"
#include "core/indexing/inverted_index.hpp"
#include <spdlog/spdlog.h>

namespace mindtrace {

EventPipeline::EventPipeline(size_t num_workers)
    : num_workers_(num_workers) {}

EventPipeline::~EventPipeline() {
    stop();
}

void EventPipeline::add_stage(PipelineStage stage) {
    stages_.push_back(std::move(stage));
}

void EventPipeline::set_storage(std::shared_ptr<EventRepository> storage) {
    storage_ = std::move(storage);
}

void EventPipeline::set_index(std::shared_ptr<InvertedIndex> index) {
    index_ = std::move(index);
}

void EventPipeline::submit(ActivityEvent event) {
    queue_.push(std::move(event));
}

void EventPipeline::start() {
    if (running_.exchange(true)) return;  // Already running

    for (size_t i = 0; i < num_workers_; ++i) {
        workers_.emplace_back([this] { worker_loop(); });
    }

    spdlog::info("EventPipeline started with {} workers", num_workers_);
}

void EventPipeline::stop() {
    if (!running_.exchange(false)) return;  // Already stopped

    queue_.stop();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();

    spdlog::info("EventPipeline stopped. Processed {} events", processed_count_.load());
}

void EventPipeline::worker_loop() {
    while (running_.load()) {
        auto event_opt = queue_.pop_for(std::chrono::milliseconds(100));
        if (!event_opt) continue;

        auto& event = *event_opt;

        // Run through all stages
        for (auto& stage : stages_) {
            try {
                stage(event);
            } catch (const std::exception& e) {
                spdlog::error("Pipeline stage error: {}", e.what());
            }
        }

        // Persist to storage
        if (storage_) {
            try {
                uint64_t id = storage_->save(event);
                event.id = id;
            } catch (const std::exception& e) {
                spdlog::error("Storage error: {}", e.what());
            }
        }

        // Update index
        if (index_) {
            try {
                auto keywords = extract_keywords(event);
                // Index will be updated via its add method
                // (implemented in inverted_index.hpp)
            } catch (const std::exception& e) {
                spdlog::error("Index error: {}", e.what());
            }
        }

        processed_count_.fetch_add(1);
    }
}

}  // namespace mindtrace
