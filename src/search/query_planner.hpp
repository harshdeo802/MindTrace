#ifndef MINDTRACE_SEARCH_QUERY_PLANNER_HPP
#define MINDTRACE_SEARCH_QUERY_PLANNER_HPP

#include "search/query_parser.hpp"
#include "core/indexing/inverted_index.hpp"
#include <vector>
#include <string>
#include <memory>

namespace mindtrace {

/// Describes which indexes to query and in what order.
struct QueryPlan {
    bool use_keyword_index = false;
    bool use_time_index = false;
    bool use_type_filter = false;
    bool use_app_index = false;

    std::vector<std::string> keywords;
    uint64_t time_start = 0;
    uint64_t time_end = 0;
    EventType type_filter = EventType::Unknown;
    std::string app_filter;
};

/// Converts a QuerySpec into an executable QueryPlan.
class QueryPlanner {
public:
    [[nodiscard]] QueryPlan plan(const QuerySpec& spec) const;
};

}  // namespace mindtrace

#endif  // MINDTRACE_SEARCH_QUERY_PLANNER_HPP
