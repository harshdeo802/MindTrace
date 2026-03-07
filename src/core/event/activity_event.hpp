#ifndef MINDTRACE_CORE_EVENT_ACTIVITY_EVENT_HPP
#define MINDTRACE_CORE_EVENT_ACTIVITY_EVENT_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

namespace mindtrace {

/// Types of activity events captured by the system.
enum class EventType : uint8_t {
    FileOpened = 0,
    FileModified,
    BrowserVisit,
    WindowFocus,
    ClipboardCopy,
    ScreenshotCaptured,
    Unknown
};

/// Convert EventType to its string representation.
[[nodiscard]] std::string event_type_to_string(EventType type);

/// Convert a string to EventType. Returns Unknown on failure.
[[nodiscard]] EventType string_to_event_type(const std::string& str);

/// Core data structure representing a single user activity event.
struct ActivityEvent {
    uint64_t id = 0;
    uint64_t timestamp = 0;  // Unix epoch seconds
    EventType type = EventType::Unknown;
    std::string source;       // e.g. file path, URL, app name
    std::string content;      // description or content snippet
    std::unordered_map<std::string, std::string> metadata;

    /// Serialize to JSON
    [[nodiscard]] nlohmann::json to_json() const;

    /// Deserialize from JSON
    static ActivityEvent from_json(const nlohmann::json& j);

    bool operator==(const ActivityEvent& other) const = default;
};

/// Extract keywords from an event's content and source fields.
[[nodiscard]] std::vector<std::string> extract_keywords(const ActivityEvent& event);

}  // namespace mindtrace

#endif  // MINDTRACE_CORE_EVENT_ACTIVITY_EVENT_HPP
