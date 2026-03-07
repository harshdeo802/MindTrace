#ifndef MINDTRACE_SEARCH_QUERY_EXECUTOR_HPP
#define MINDTRACE_SEARCH_QUERY_EXECUTOR_HPP

#include "search/query_planner.hpp"
#include "core/storage/event_repository.hpp"
#include "core/indexing/inverted_index.hpp"
#include <memory>
#include <vector>

namespace mindtrace {

/// Executes a QueryPlan against the index and storage to return matching events.
class QueryExecutor {
public:
    QueryExecutor(std::shared_ptr<EventRepository> storage,
                  std::shared_ptr<InvertedIndex> index);

    /// Execute a plan and return matching events, sorted by timestamp descending.
    [[nodiscard]] std::vector<ActivityEvent> execute(const QueryPlan& plan) const;

    /// Convenience: parse a natural language query and execute it.
    [[nodiscard]] std::vector<ActivityEvent> search(const std::string& query) const;

private:
    std::shared_ptr<EventRepository> storage_;
    std::shared_ptr<InvertedIndex> index_;
    QueryParser parser_;
    QueryPlanner planner_;
};

}  // namespace mindtrace

#endif  // MINDTRACE_SEARCH_QUERY_EXECUTOR_HPP
