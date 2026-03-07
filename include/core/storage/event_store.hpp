#ifndef MINDTRACE_CORE_STORAGE_EVENT_STORE_HPP
#define MINDTRACE_CORE_STORAGE_EVENT_STORE_HPP

#include "core/storage/event_repository.hpp"
#include <string>
#include <mutex>

#include <sqlite3.h>

namespace mindtrace {

/// SQLite-backed implementation of EventRepository.
class EventStore : public EventRepository {
public:
    /// Construct with a database path. Use ":memory:" for in-memory DB.
    explicit EventStore(const std::string& db_path = ":memory:");
    ~EventStore() override;

    // Non-copyable, movable
    EventStore(const EventStore&) = delete;
    EventStore& operator=(const EventStore&) = delete;
    EventStore(EventStore&& other) noexcept;
    EventStore& operator=(EventStore&& other) noexcept;

    uint64_t save(const ActivityEvent& event) override;
    void save_batch(const std::vector<ActivityEvent>& events) override;
    [[nodiscard]] std::optional<ActivityEvent> find_by_id(uint64_t id) const override;
    [[nodiscard]] std::vector<ActivityEvent> find_by_time_range(
        uint64_t start, uint64_t end) const override;
    [[nodiscard]] std::vector<ActivityEvent> find_by_type(EventType type) const override;
    [[nodiscard]] std::vector<ActivityEvent> find_by_keyword(
        const std::string& keyword) const override;
    [[nodiscard]] uint64_t count() const override;
    void clear() override;

private:
    void init_schema();
    [[nodiscard]] ActivityEvent row_to_event(sqlite3_stmt* stmt) const;

    sqlite3* db_ = nullptr;
    mutable std::mutex mutex_;
};

}  // namespace mindtrace

#endif  // MINDTRACE_CORE_STORAGE_EVENT_STORE_HPP
