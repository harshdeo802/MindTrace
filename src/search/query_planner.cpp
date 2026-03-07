#include "search/query_planner.hpp"

namespace mindtrace {

QueryPlan QueryPlanner::plan(const QuerySpec& spec) const {
    QueryPlan plan;

    // Keywords
    if (!spec.keywords.empty()) {
        plan.use_keyword_index = true;
        plan.keywords = spec.keywords;
    }

    // Time range
    if (spec.time_start && spec.time_end) {
        plan.use_time_index = true;
        plan.time_start = *spec.time_start;
        plan.time_end = *spec.time_end;
    }

    // Event type
    if (spec.event_type) {
        plan.use_type_filter = true;
        plan.type_filter = *spec.event_type;
    }

    // Application
    if (spec.application) {
        plan.use_app_index = true;
        plan.app_filter = *spec.application;
    }

    return plan;
}

}  // namespace mindtrace
