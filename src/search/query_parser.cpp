#include "search/query_parser.hpp"

#include <algorithm>
#include <sstream>
#include <cctype>
#include <chrono>
#include <ctime>
#include <regex>
#include <unordered_set>

namespace mindtrace {

uint64_t QueryParser::now() const {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
}

uint64_t QueryParser::today_start() const {
    auto n = now();
    // Round down to start of day (UTC)
    return (n / 86400) * 86400;
}

QuerySpec QueryParser::parse(const std::string& query) const {
    QuerySpec spec;
    spec.raw_query = query;

    // Lowercase the query for pattern matching
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(),
        [](unsigned char c) { return std::tolower(c); });

    resolve_time_expression(lower_query, spec);
    resolve_event_type(lower_query, spec);
    extract_query_keywords(lower_query, spec);

    return spec;
}

void QueryParser::resolve_time_expression(const std::string& query, QuerySpec& spec) const {
    uint64_t ts = today_start();

    if (query.find("today") != std::string::npos) {
        spec.time_start = ts;
        spec.time_end = ts + 86400 - 1;
    } else if (query.find("yesterday") != std::string::npos) {
        spec.time_start = ts - 86400;
        spec.time_end = ts - 1;
    } else if (query.find("this week") != std::string::npos) {
        // Go back to start of the week (Monday)
        auto now_time = std::chrono::system_clock::from_time_t(static_cast<time_t>(ts));
        auto time_t_val = std::chrono::system_clock::to_time_t(now_time);
        std::tm tm_val{};
#ifdef _WIN32
        gmtime_s(&tm_val, &time_t_val);
#else
        gmtime_r(&time_t_val, &tm_val);
#endif
        int day_of_week = tm_val.tm_wday;
        if (day_of_week == 0) day_of_week = 7;  // Sunday = 7
        uint64_t week_start = ts - static_cast<uint64_t>((day_of_week - 1)) * 86400;
        spec.time_start = week_start;
        spec.time_end = now();
    } else if (query.find("last week") != std::string::npos) {
        auto now_time = std::chrono::system_clock::from_time_t(static_cast<time_t>(ts));
        auto time_t_val = std::chrono::system_clock::to_time_t(now_time);
        std::tm tm_val{};
#ifdef _WIN32
        gmtime_s(&tm_val, &time_t_val);
#else
        gmtime_r(&time_t_val, &tm_val);
#endif
        int day_of_week = tm_val.tm_wday;
        if (day_of_week == 0) day_of_week = 7;
        uint64_t this_week_start = ts - static_cast<uint64_t>((day_of_week - 1)) * 86400;
        spec.time_start = this_week_start - 7 * 86400;
        spec.time_end = this_week_start - 1;
    } else {
        // Check for day names: "last monday", "last tuesday", etc.
        static const std::vector<std::pair<std::string, int>> days = {
            {"monday", 1}, {"tuesday", 2}, {"wednesday", 3},
            {"thursday", 4}, {"friday", 5}, {"saturday", 6}, {"sunday", 0}
        };

        for (const auto& [day_name, wday] : days) {
            if (query.find("last " + day_name) != std::string::npos) {
                auto time_t_val = static_cast<time_t>(ts);
                std::tm tm_val{};
#ifdef _WIN32
                gmtime_s(&tm_val, &time_t_val);
#else
                gmtime_r(&time_t_val, &tm_val);
#endif
                int current_wday = tm_val.tm_wday;
                int diff = current_wday - wday;
                if (diff <= 0) diff += 7;
                uint64_t target_day_start = ts - static_cast<uint64_t>(diff) * 86400;
                spec.time_start = target_day_start;
                spec.time_end = target_day_start + 86400 - 1;
                break;
            }
        }
    }

    // Handle "work hours" (9:00 - 17:00)
    if (query.find("work hours") != std::string::npos ||
        query.find("during work") != std::string::npos) {
        if (spec.time_start) {
            spec.time_start = *spec.time_start + 9 * 3600;  // 9 AM
            spec.time_end = *spec.time_start + 8 * 3600 - 1;  // 5 PM
        }
    }
}

void QueryParser::resolve_event_type(const std::string& query, QuerySpec& spec) const {
    if (query.find("file") != std::string::npos) {
        if (query.find("edit") != std::string::npos || query.find("modif") != std::string::npos) {
            spec.event_type = EventType::FileModified;
        } else if (query.find("open") != std::string::npos) {
            spec.event_type = EventType::FileOpened;
        }
    } else if (query.find("website") != std::string::npos ||
               query.find("browser") != std::string::npos ||
               query.find("visit") != std::string::npos) {
        spec.event_type = EventType::BrowserVisit;
    } else if (query.find("clipboard") != std::string::npos ||
               query.find("copied") != std::string::npos ||
               query.find("copy") != std::string::npos) {
        spec.event_type = EventType::ClipboardCopy;
    } else if (query.find("window") != std::string::npos ||
               query.find("focus") != std::string::npos) {
        spec.event_type = EventType::WindowFocus;
    } else if (query.find("screenshot") != std::string::npos) {
        spec.event_type = EventType::ScreenshotCaptured;
    }
}

void QueryParser::extract_query_keywords(const std::string& query, QuerySpec& spec) const {
    // Stop words to skip
    static const std::unordered_set<std::string> stop_words = {
        "what", "was", "i", "doing", "the", "a", "an", "in", "on", "at",
        "to", "for", "of", "is", "are", "were", "has", "have", "had",
        "do", "does", "did", "will", "would", "could", "should", "can",
        "my", "me", "this", "that", "these", "those", "it", "its",
        "today", "yesterday", "last", "week", "during", "work", "hours",
        "files", "file", "edited", "opened", "websites", "visited",
        "browser", "window", "clipboard", "screenshot", "monday", "tuesday",
        "wednesday", "thursday", "friday", "saturday", "sunday"
    };

    std::istringstream stream(query);
    std::string word;
    while (stream >> word) {
        // Clean punctuation
        std::string clean;
        for (char c : word) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                clean += c;
            }
        }
        if (!clean.empty() && clean.size() > 1 && stop_words.find(clean) == stop_words.end()) {
            spec.keywords.push_back(clean);
        }
    }
}

}  // namespace mindtrace
