#ifndef MINDTRACE_TIMELINE_TIMELINE_BUILDER_HPP
#define MINDTRACE_TIMELINE_TIMELINE_BUILDER_HPP

#include "core/event/activity_event.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include <chrono>

namespace mindtrace {

/// Represents a group of related events within a time window.
struct Session {
    uint64_t start_time = 0;
    uint64_t end_time = 0;
    std::string primary_app;  // most frequent source in the session
    std::vector<ActivityEvent> events;

    /// Duration in seconds.
    [[nodiscard]] uint64_t duration() const { return end_time - start_time; }
};

/// A timeline is a sequence of sessions for a given time period.
struct Timeline {
    uint64_t period_start = 0;
    uint64_t period_end = 0;
    std::vector<Session> sessions;

    /// Total number of events across all sessions.
    [[nodiscard]] size_t total_events() const {
        size_t count = 0;
        for (const auto& s : sessions) count += s.events.size();
        return count;
    }
};

/// Builds timelines by grouping events into sessions based on time proximity.
class TimelineBuilder {
public:
    /// Gap threshold: if two consecutive events are more than this apart,
    /// they belong to different sessions. Default: 30 minutes.
    explicit TimelineBuilder(uint64_t gap_threshold_seconds = 1800);

    /// Build a timeline from a sorted (by timestamp) list of events.
    [[nodiscard]] Timeline build(const std::vector<ActivityEvent>& events) const;

    /// Build a timeline for a specific time range.
    [[nodiscard]] Timeline build(const std::vector<ActivityEvent>& events,
                                 uint64_t period_start, uint64_t period_end) const;

    /// Set the gap threshold.
    void set_gap_threshold(uint64_t seconds) { gap_threshold_ = seconds; }

private:
    /// Determine the primary app for a session.
    [[nodiscard]] std::string determine_primary_app(
        const std::vector<ActivityEvent>& events) const;

    uint64_t gap_threshold_;
};

}  // namespace mindtrace

#endif  // MINDTRACE_TIMELINE_TIMELINE_BUILDER_HPP
