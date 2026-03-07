#ifndef MINDTRACE_CORE_PIPELINE_EVENT_PIPELINE_HPP
#define MINDTRACE_CORE_PIPELINE_EVENT_PIPELINE_HPP

#include "core/event/activity_event.hpp"
#include "core/pipeline/concurrent_queue.hpp"
#include "core/storage/event_repository.hpp"
#include <functional>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>

namespace mindtrace {

// Forward declare
class InvertedIndex;

/// Pipeline stage function: transforms/enriches an event in-place.
using PipelineStage = std::function<void(ActivityEvent&)>;

/// Multi-stage event processing pipeline running on a background thread pool.
class EventPipeline {
public:
    /// Construct a pipeline with the given number of worker threads.
    explicit EventPipeline(size_t num_workers = 2);
    ~EventPipeline();

    // Non-copyable
    EventPipeline(const EventPipeline&) = delete;
    EventPipeline& operator=(const EventPipeline&) = delete;

    /// Add a processing stage to the pipeline.
    void add_stage(PipelineStage stage);

    /// Set the storage backend where processed events are persisted.
    void set_storage(std::shared_ptr<EventRepository> storage);

    /// Set the index to update on each processed event.
    void set_index(std::shared_ptr<InvertedIndex> index);

    /// Submit an event for processing.
    void submit(ActivityEvent event);

    /// Start the worker threads.
    void start();

    /// Stop the pipeline gracefully.
    void stop();

    /// Get the number of events processed so far.
    [[nodiscard]] uint64_t processed_count() const { return processed_count_.load(); }

    /// Check if the pipeline is running.
    [[nodiscard]] bool is_running() const { return running_.load(); }

private:
    void worker_loop();

    ConcurrentQueue<ActivityEvent> queue_;
    std::vector<PipelineStage> stages_;
    std::vector<std::thread> workers_;
    std::shared_ptr<EventRepository> storage_;
    std::shared_ptr<InvertedIndex> index_;
    std::atomic<uint64_t> processed_count_{0};
    std::atomic<bool> running_{false};
    size_t num_workers_;
};

}  // namespace mindtrace

#endif  // MINDTRACE_CORE_PIPELINE_EVENT_PIPELINE_HPP
