#ifndef MINDTRACE_COLLECTORS_COLLECTOR_BASE_HPP
#define MINDTRACE_COLLECTORS_COLLECTOR_BASE_HPP

#include "core/event/activity_event.hpp"
#include <functional>
#include <atomic>
#include <string>

namespace mindtrace {

/// Callback type for when a collector emits an event.
using EventCallback = std::function<void(ActivityEvent)>;

/// Abstract base class for all activity collectors.
class CollectorBase {
public:
    explicit CollectorBase(std::string name) : name_(std::move(name)) {}
    virtual ~CollectorBase() = default;

    /// Start collecting events.
    virtual void start() = 0;

    /// Stop collecting events.
    virtual void stop() = 0;

    /// Register the callback for emitting events.
    void set_callback(EventCallback callback) {
        callback_ = std::move(callback);
    }

    /// Get the collector name.
    [[nodiscard]] const std::string& name() const { return name_; }

    /// Check if the collector is running.
    [[nodiscard]] bool is_running() const { return running_.load(); }

protected:
    /// Emit an event via the registered callback.
    void emit(ActivityEvent event) {
        if (callback_) {
            callback_(std::move(event));
        }
    }

    EventCallback callback_;
    std::atomic<bool> running_{false};
    std::string name_;
};

}  // namespace mindtrace

#endif  // MINDTRACE_COLLECTORS_COLLECTOR_BASE_HPP
