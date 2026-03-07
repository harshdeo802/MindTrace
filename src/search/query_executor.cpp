#include "search/query_executor.hpp"

#include <algorithm>
#include <chrono>
#include <unordered_set>

namespace mindtrace {

namespace {
uint64_t now_epoch() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
}
}  // namespace

QueryExecutor::QueryExecutor(
    std::shared_ptr<EventRepository> storage,
    std::shared_ptr<InvertedIndex> index)
    : storage_(std::move(storage)), index_(std::move(index)) {}

std::vector<ActivityEvent> QueryExecutor::execute(const QueryPlan& plan) const {
    // Collect candidate event IDs from indexes
    std::vector<std::unordered_set<uint64_t>> candidate_sets;

    if (plan.use_keyword_index && index_) {
        std::unordered_set<uint64_t> kw_ids;
        for (const auto& kw : plan.keywords) {
            auto results = index_->search_keyword(kw);
            kw_ids.insert(results.begin(), results.end());
        }
        candidate_sets.push_back(std::move(kw_ids));
    }

    if (plan.use_time_index && index_) {
        auto time_ids = index_->search_time_range(plan.time_start, plan.time_end);
        candidate_sets.push_back(std::move(time_ids));
    }

    if (plan.use_app_index && index_) {
        auto app_ids = index_->search_application(plan.app_filter);
        candidate_sets.push_back(std::move(app_ids));
    }

    // Intersect all candidate sets
    std::unordered_set<uint64_t> final_ids;
    if (candidate_sets.empty()) {
        // No index constraints — fall back to storage queries
        if (plan.use_type_filter && storage_) {
            auto events = storage_->find_by_type(plan.type_filter);
            std::sort(events.begin(), events.end(),
                [](const auto& a, const auto& b) { return a.timestamp > b.timestamp; });
            return events;
        }
        // No filters at all — return recent events
        if (storage_) {
            auto n = now_epoch();
            auto events = storage_->find_by_time_range(0, n);
            std::sort(events.begin(), events.end(),
                [](const auto& a, const auto& b) { return a.timestamp > b.timestamp; });
            if (events.size() > 100) events.resize(100);
            return events;
        }
        return {};
    }

    // Start with the smallest set for efficiency
    std::sort(candidate_sets.begin(), candidate_sets.end(),
        [](const auto& a, const auto& b) { return a.size() < b.size(); });

    final_ids = candidate_sets[0];
    for (size_t i = 1; i < candidate_sets.size(); ++i) {
        std::unordered_set<uint64_t> intersection;
        for (uint64_t id : final_ids) {
            if (candidate_sets[i].count(id)) {
                intersection.insert(id);
            }
        }
        final_ids = std::move(intersection);
    }

    // Fetch full events from storage
    std::vector<ActivityEvent> results;
    results.reserve(final_ids.size());
    for (uint64_t id : final_ids) {
        auto event = storage_->find_by_id(id);
        if (event) {
            // Apply type filter if needed
            if (plan.use_type_filter && event->type != plan.type_filter) {
                continue;
            }
            results.push_back(std::move(*event));
        }
    }

    // Sort by timestamp descending (most recent first)
    std::sort(results.begin(), results.end(),
        [](const auto& a, const auto& b) { return a.timestamp > b.timestamp; });

    return results;
}

std::vector<ActivityEvent> QueryExecutor::search(const std::string& query) const {
    auto spec = parser_.parse(query);
    auto plan = planner_.plan(spec);
    return execute(plan);
}

}  // namespace mindtrace
