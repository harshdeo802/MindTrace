#include "timeline/timeline_builder.hpp"

#include <algorithm>
#include <unordered_map>

namespace mindtrace {

TimelineBuilder::TimelineBuilder(uint64_t gap_threshold_seconds)
    : gap_threshold_(gap_threshold_seconds) {}

Timeline TimelineBuilder::build(const std::vector<ActivityEvent>& events) const {
    if (events.empty()) return {};

    // Ensure events are sorted by timestamp
    auto sorted = events;
    std::sort(sorted.begin(), sorted.end(),
        [](const auto& a, const auto& b) { return a.timestamp < b.timestamp; });

    Timeline timeline;
    timeline.period_start = sorted.front().timestamp;
    timeline.period_end = sorted.back().timestamp;

    Session current_session;
    current_session.start_time = sorted[0].timestamp;
    current_session.events.push_back(sorted[0]);

    for (size_t i = 1; i < sorted.size(); ++i) {
        uint64_t gap = sorted[i].timestamp - sorted[i - 1].timestamp;

        if (gap > gap_threshold_) {
            // Close current session
            current_session.end_time = sorted[i - 1].timestamp;
            current_session.primary_app = determine_primary_app(current_session.events);
            timeline.sessions.push_back(std::move(current_session));

            // Start new session
            current_session = Session{};
            current_session.start_time = sorted[i].timestamp;
        }

        current_session.events.push_back(sorted[i]);
    }

    // Close the last session
    current_session.end_time = sorted.back().timestamp;
    current_session.primary_app = determine_primary_app(current_session.events);
    timeline.sessions.push_back(std::move(current_session));

    return timeline;
}

Timeline TimelineBuilder::build(const std::vector<ActivityEvent>& events,
                                 uint64_t period_start, uint64_t period_end) const {
    // Filter events to the time range
    std::vector<ActivityEvent> filtered;
    for (const auto& event : events) {
        if (event.timestamp >= period_start && event.timestamp <= period_end) {
            filtered.push_back(event);
        }
    }

    auto timeline = build(filtered);
    timeline.period_start = period_start;
    timeline.period_end = period_end;
    return timeline;
}

std::string TimelineBuilder::determine_primary_app(
    const std::vector<ActivityEvent>& events) const {
    std::unordered_map<std::string, size_t> app_counts;

    for (const auto& event : events) {
        std::string app = event.source;
        // Extract filename from path
        auto pos = app.find_last_of("/\\");
        if (pos != std::string::npos) {
            app = app.substr(pos + 1);
        }
        app_counts[app]++;
    }

    std::string primary;
    size_t max_count = 0;
    for (const auto& [app, count] : app_counts) {
        if (count > max_count) {
            max_count = count;
            primary = app;
        }
    }

    return primary;
}

}  // namespace mindtrace
