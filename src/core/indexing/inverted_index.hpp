#ifndef MINDTRACE_CORE_INDEXING_INVERTED_INDEX_HPP
#define MINDTRACE_CORE_INDEXING_INVERTED_INDEX_HPP

#include "core/event/activity_event.hpp"
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>
#include <string>
#include <vector>

namespace mindtrace {

/// In-memory inverted index for fast event lookup by keyword, time, and application.
class InvertedIndex {
public:
    InvertedIndex() = default;

    /// Add an event to all indexes.
    void add(const ActivityEvent& event);

    /// Search by keyword (case-insensitive partial match).
    [[nodiscard]] std::unordered_set<uint64_t> search_keyword(const std::string& keyword) const;

    /// Search by time range [start, end] (unix epoch seconds).
    [[nodiscard]] std::unordered_set<uint64_t> search_time_range(
        uint64_t start, uint64_t end) const;

    /// Search by application/source name.
    [[nodiscard]] std::unordered_set<uint64_t> search_application(
        const std::string& app_name) const;

    /// Get total number of indexed events.
    [[nodiscard]] size_t event_count() const;

    /// Get total number of unique keywords.
    [[nodiscard]] size_t keyword_count() const;

    /// Clear all indexes.
    void clear();

private:
    /// Keyword → set of event IDs
    std::unordered_map<std::string, std::unordered_set<uint64_t>> keyword_index_;

    /// Hour bucket (timestamp / 3600) → set of event IDs
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> time_index_;

    /// Application/source → set of event IDs
    std::unordered_map<std::string, std::unordered_set<uint64_t>> app_index_;

    /// Track all indexed event IDs
    std::unordered_set<uint64_t> all_events_;

    mutable std::shared_mutex mutex_;
};

}  // namespace mindtrace

#endif  // MINDTRACE_CORE_INDEXING_INVERTED_INDEX_HPP
