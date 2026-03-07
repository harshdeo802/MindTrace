#include "core/event/activity_event.hpp"

#include <algorithm>
#include <sstream>
#include <cctype>

namespace mindtrace {

std::string event_type_to_string(EventType type) {
    switch (type) {
        case EventType::FileOpened:          return "FileOpened";
        case EventType::FileModified:        return "FileModified";
        case EventType::BrowserVisit:        return "BrowserVisit";
        case EventType::WindowFocus:         return "WindowFocus";
        case EventType::ClipboardCopy:       return "ClipboardCopy";
        case EventType::ScreenshotCaptured:  return "ScreenshotCaptured";
        default:                             return "Unknown";
    }
}

EventType string_to_event_type(const std::string& str) {
    if (str == "FileOpened")          return EventType::FileOpened;
    if (str == "FileModified")        return EventType::FileModified;
    if (str == "BrowserVisit")        return EventType::BrowserVisit;
    if (str == "WindowFocus")         return EventType::WindowFocus;
    if (str == "ClipboardCopy")       return EventType::ClipboardCopy;
    if (str == "ScreenshotCaptured")  return EventType::ScreenshotCaptured;
    return EventType::Unknown;
}

nlohmann::json ActivityEvent::to_json() const {
    nlohmann::json j;
    j["id"] = id;
    j["timestamp"] = timestamp;
    j["type"] = event_type_to_string(type);
    j["source"] = source;
    j["content"] = content;
    j["metadata"] = metadata;
    return j;
}

ActivityEvent ActivityEvent::from_json(const nlohmann::json& j) {
    ActivityEvent event;
    event.id = j.value("id", uint64_t{0});
    event.timestamp = j.at("timestamp").get<uint64_t>();
    event.type = string_to_event_type(j.at("type").get<std::string>());
    event.source = j.at("source").get<std::string>();
    event.content = j.at("content").get<std::string>();
    if (j.contains("metadata")) {
        event.metadata = j.at("metadata").get<std::unordered_map<std::string, std::string>>();
    }
    return event;
}

std::vector<std::string> extract_keywords(const ActivityEvent& event) {
    std::vector<std::string> keywords;
    auto tokenize = [&](const std::string& text) {
        std::istringstream stream(text);
        std::string word;
        while (stream >> word) {
            // Lowercase and strip punctuation
            std::string clean;
            clean.reserve(word.size());
            for (char c : word) {
                if (std::isalnum(static_cast<unsigned char>(c))) {
                    clean += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
            }
            if (!clean.empty() && clean.size() > 1) {
                keywords.push_back(std::move(clean));
            }
        }
    };

    tokenize(event.content);
    tokenize(event.source);

    // Add event type as keyword
    std::string type_str = event_type_to_string(event.type);
    std::transform(type_str.begin(), type_str.end(), type_str.begin(),
        [](unsigned char c) { return std::tolower(c); });
    keywords.push_back(type_str);

    // Deduplicate
    std::sort(keywords.begin(), keywords.end());
    keywords.erase(std::unique(keywords.begin(), keywords.end()), keywords.end());

    return keywords;
}

}  // namespace mindtrace
