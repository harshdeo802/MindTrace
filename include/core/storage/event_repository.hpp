#ifndef MINDTRACE_CORE_STORAGE_EVENT_REPOSITORY_HPP
#define MINDTRACE_CORE_STORAGE_EVENT_REPOSITORY_HPP

#include "core/event/activity_event.hpp"
#include <vector>
#include <optional>
#include <memory>

namespace mindtrace {

/// Abstract interface for event persistence.
class EventRepository {
public:
    virtual ~EventRepository() = default;

    /// Save an event; returns the assigned ID.
    virtual uint64_t save(const ActivityEvent& event) = 0;

    /// Save a batch of events.
    virtual void save_batch(const std::vector<ActivityEvent>& events) = 0;

    /// Find an event by its ID.
    [[nodiscard]] virtual std::optional<ActivityEvent> find_by_id(uint64_t id) const = 0;

    /// Find events within a timestamp range [start, end].
    [[nodiscard]] virtual std::vector<ActivityEvent> find_by_time_range(
        uint64_t start, uint64_t end) const = 0;

    /// Find events by type.
    [[nodiscard]] virtual std::vector<ActivityEvent> find_by_type(EventType type) const = 0;

    /// Full-text keyword search on content and source fields.
    [[nodiscard]] virtual std::vector<ActivityEvent> find_by_keyword(
        const std::string& keyword) const = 0;

    /// Get the total number of stored events.
    [[nodiscard]] virtual uint64_t count() const = 0;

    /// Delete all events.
    virtual void clear() = 0;
};

}  // namespace mindtrace

#endif  // MINDTRACE_CORE_STORAGE_EVENT_REPOSITORY_HPP
