#ifndef MINDTRACE_CORE_PIPELINE_CONCURRENT_QUEUE_HPP
#define MINDTRACE_CORE_PIPELINE_CONCURRENT_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>

namespace mindtrace {

/// Thread-safe concurrent queue with blocking pop and timed wait.
template <typename T>
class ConcurrentQueue {
public:
    ConcurrentQueue() = default;

    /// Push an item to the back of the queue.
    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(item));
        }
        cv_.notify_one();
    }

    /// Try to pop an item. Returns nullopt if queue is empty.
    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return std::nullopt;
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    /// Blocking pop — waits until an item is available or the queue is stopped.
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty() || stopped_; });
        if (queue_.empty()) return std::nullopt;
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    /// Timed pop — waits up to the specified duration.
    template <typename Duration>
    std::optional<T> pop_for(Duration timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!cv_.wait_for(lock, timeout, [this] { return !queue_.empty() || stopped_; })) {
            return std::nullopt;
        }
        if (queue_.empty()) return std::nullopt;
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    /// Signal all waiting threads to stop.
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        cv_.notify_all();
    }

    /// Check if the queue is empty.
    [[nodiscard]] bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /// Get the current size of the queue.
    [[nodiscard]] size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /// Check if the queue has been stopped.
    [[nodiscard]] bool is_stopped() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stopped_;
    }

private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool stopped_ = false;
};

}  // namespace mindtrace

#endif  // MINDTRACE_CORE_PIPELINE_CONCURRENT_QUEUE_HPP
