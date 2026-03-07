#ifndef MINDTRACE_SEARCH_QUERY_PARSER_HPP
#define MINDTRACE_SEARCH_QUERY_PARSER_HPP

#include "core/event/activity_event.hpp"
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace mindtrace {

/// Parsed query specification from natural language.
struct QuerySpec {
    /// Keywords to search for.
    std::vector<std::string> keywords;

    /// Time range filter (unix epoch seconds).
    std::optional<uint64_t> time_start;
    std::optional<uint64_t> time_end;

    /// Event type filter.
    std::optional<EventType> event_type;

    /// Application/source filter.
    std::optional<std::string> application;

    /// Raw query string.
    std::string raw_query;
};

/// Parses natural language queries into structured QuerySpec.
class QueryParser {
public:
    /// Parse a natural language query string.
    [[nodiscard]] QuerySpec parse(const std::string& query) const;

private:
    /// Resolve time expressions like "today", "yesterday", "last tuesday"
    void resolve_time_expression(const std::string& query, QuerySpec& spec) const;

    /// Detect event type from query words
    void resolve_event_type(const std::string& query, QuerySpec& spec) const;

    /// Extract remaining keywords
    void extract_query_keywords(const std::string& query, QuerySpec& spec) const;

    /// Get current timestamp.
    [[nodiscard]] uint64_t now() const;

    /// Get start of today (midnight UTC).
    [[nodiscard]] uint64_t today_start() const;
};

}  // namespace mindtrace

#endif  // MINDTRACE_SEARCH_QUERY_PARSER_HPP
