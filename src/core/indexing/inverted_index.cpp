#include "core/indexing/inverted_index.hpp"

#include <algorithm>
#include <cctype>
#include <mutex>

namespace mindtrace {

void InvertedIndex::add(const ActivityEvent& event) {
    std::unique_lock lock(mutex_);

    all_events_.insert(event.id);

    // Extract and index keywords
    auto keywords = extract_keywords(event);
    for (const auto& kw : keywords) {
        keyword_index_[kw].insert(event.id);
    }

    // Time index: bucket by hour
    uint64_t hour_bucket = event.timestamp / 3600;
    time_index_[hour_bucket].insert(event.id);

    // Application index: use source as app identifier
    if (!event.source.empty()) {
        std::string app = event.source;
        // Extract app name from path (take last component)
        auto pos = app.find_last_of("/\\");
        if (pos != std::string::npos) {
            app = app.substr(pos + 1);
        }
        // Lowercase
        std::transform(app.begin(), app.end(), app.begin(),
            [](unsigned char c) { return std::tolower(c); });
        app_index_[app].insert(event.id);
    }
}

std::unordered_set<uint64_t> InvertedIndex::search_keyword(
    const std::string& keyword) const {
    std::shared_lock lock(mutex_);

    std::string lower_kw = keyword;
    std::transform(lower_kw.begin(), lower_kw.end(), lower_kw.begin(),
        [](unsigned char c) { return std::tolower(c); });

    std::unordered_set<uint64_t> results;

    // Exact match first
    auto it = keyword_index_.find(lower_kw);
    if (it != keyword_index_.end()) {
        results = it->second;
    }

    // Partial match: check if keyword is a substring of indexed keywords
    for (const auto& [indexed_kw, ids] : keyword_index_) {
        if (indexed_kw.find(lower_kw) != std::string::npos) {
            results.insert(ids.begin(), ids.end());
        }
    }

    return results;
}

std::unordered_set<uint64_t> InvertedIndex::search_time_range(
    uint64_t start, uint64_t end) const {
    std::shared_lock lock(mutex_);

    uint64_t start_bucket = start / 3600;
    uint64_t end_bucket = end / 3600;

    std::unordered_set<uint64_t> results;
    for (uint64_t bucket = start_bucket; bucket <= end_bucket; ++bucket) {
        auto it = time_index_.find(bucket);
        if (it != time_index_.end()) {
            results.insert(it->second.begin(), it->second.end());
        }
    }

    return results;
}

std::unordered_set<uint64_t> InvertedIndex::search_application(
    const std::string& app_name) const {
    std::shared_lock lock(mutex_);

    std::string lower_app = app_name;
    std::transform(lower_app.begin(), lower_app.end(), lower_app.begin(),
        [](unsigned char c) { return std::tolower(c); });

    std::unordered_set<uint64_t> results;

    for (const auto& [app, ids] : app_index_) {
        if (app.find(lower_app) != std::string::npos) {
            results.insert(ids.begin(), ids.end());
        }
    }

    return results;
}

size_t InvertedIndex::event_count() const {
    std::shared_lock lock(mutex_);
    return all_events_.size();
}

size_t InvertedIndex::keyword_count() const {
    std::shared_lock lock(mutex_);
    return keyword_index_.size();
}

void InvertedIndex::clear() {
    std::unique_lock lock(mutex_);
    keyword_index_.clear();
    time_index_.clear();
    app_index_.clear();
    all_events_.clear();
}

}  // namespace mindtrace
