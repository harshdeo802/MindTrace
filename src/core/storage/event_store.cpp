#include "core/storage/event_store.hpp"

#include <sqlite3.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace mindtrace {

EventStore::EventStore(const std::string& db_path) {
    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string err = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        db_ = nullptr;
        throw std::runtime_error("Failed to open database: " + err);
    }
    // Enable WAL mode for better concurrency
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    init_schema();
}

EventStore::~EventStore() {
    if (db_) {
        sqlite3_close(db_);
    }
}

EventStore::EventStore(EventStore&& other) noexcept : db_(other.db_) {
    other.db_ = nullptr;
}

EventStore& EventStore::operator=(EventStore&& other) noexcept {
    if (this != &other) {
        if (db_) sqlite3_close(db_);
        db_ = other.db_;
        other.db_ = nullptr;
    }
    return *this;
}

void EventStore::init_schema() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS events (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp   INTEGER NOT NULL,
            type        TEXT NOT NULL,
            source      TEXT NOT NULL,
            content     TEXT NOT NULL,
            metadata_json TEXT DEFAULT '{}'
        );
        CREATE INDEX IF NOT EXISTS idx_events_timestamp ON events(timestamp);
        CREATE INDEX IF NOT EXISTS idx_events_type ON events(type);
    )";

    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string error_msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw std::runtime_error("Failed to initialize schema: " + error_msg);
    }
}

uint64_t EventStore::save(const ActivityEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = R"(
        INSERT INTO events (timestamp, type, source, content, metadata_json)
        VALUES (?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare insert: " +
            std::string(sqlite3_errmsg(db_)));
    }

    nlohmann::json meta_json(event.metadata);
    std::string meta_str = meta_json.dump();
    std::string type_str = event_type_to_string(event.type);

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(event.timestamp));
    sqlite3_bind_text(stmt, 2, type_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, event.source.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, event.content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, meta_str.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        throw std::runtime_error("Failed to insert event: " +
            std::string(sqlite3_errmsg(db_)));
    }

    return static_cast<uint64_t>(sqlite3_last_insert_rowid(db_));
}

void EventStore::save_batch(const std::vector<ActivityEvent>& events) {
    std::lock_guard<std::mutex> lock(mutex_);

    sqlite3_exec(db_, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

    const char* sql = R"(
        INSERT INTO events (timestamp, type, source, content, metadata_json)
        VALUES (?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    for (const auto& event : events) {
        nlohmann::json meta_json(event.metadata);
        std::string meta_str = meta_json.dump();
        std::string type_str = event_type_to_string(event.type);

        sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(event.timestamp));
        sqlite3_bind_text(stmt, 2, type_str.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, event.source.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, event.content.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, meta_str.c_str(), -1, SQLITE_TRANSIENT);

        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr);
}

std::optional<ActivityEvent> EventStore::find_by_id(uint64_t id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT * FROM events WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(id));

    std::optional<ActivityEvent> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = row_to_event(stmt);
    }

    sqlite3_finalize(stmt);
    return result;
}

std::vector<ActivityEvent> EventStore::find_by_time_range(
    uint64_t start, uint64_t end) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT * FROM events WHERE timestamp >= ? AND timestamp <= ? ORDER BY timestamp";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(start));
    sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(end));

    std::vector<ActivityEvent> results;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(row_to_event(stmt));
    }

    sqlite3_finalize(stmt);
    return results;
}

std::vector<ActivityEvent> EventStore::find_by_type(EventType type) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT * FROM events WHERE type = ? ORDER BY timestamp";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    std::string type_str = event_type_to_string(type);
    sqlite3_bind_text(stmt, 1, type_str.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<ActivityEvent> results;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(row_to_event(stmt));
    }

    sqlite3_finalize(stmt);
    return results;
}

std::vector<ActivityEvent> EventStore::find_by_keyword(
    const std::string& keyword) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql =
        "SELECT * FROM events WHERE content LIKE ? OR source LIKE ? ORDER BY timestamp";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    std::string pattern = "%" + keyword + "%";
    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<ActivityEvent> results;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(row_to_event(stmt));
    }

    sqlite3_finalize(stmt);
    return results;
}

uint64_t EventStore::count() const {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT COUNT(*) FROM events";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    uint64_t result = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
    }

    sqlite3_finalize(stmt);
    return result;
}

void EventStore::clear() {
    std::lock_guard<std::mutex> lock(mutex_);

    char* err = nullptr;
    sqlite3_exec(db_, "DELETE FROM events", nullptr, nullptr, &err);
    if (err) sqlite3_free(err);
}

ActivityEvent EventStore::row_to_event(sqlite3_stmt* stmt) const {
    ActivityEvent event;
    event.id = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
    event.timestamp = static_cast<uint64_t>(sqlite3_column_int64(stmt, 1));

    const char* type_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    event.type = string_to_event_type(type_text ? type_text : "");

    const char* source_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    event.source = source_text ? source_text : "";

    const char* content_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    event.content = content_text ? content_text : "";

    const char* meta_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    if (meta_text) {
        try {
            auto meta_json = nlohmann::json::parse(meta_text);
            event.metadata = meta_json.get<std::unordered_map<std::string, std::string>>();
        } catch (...) {
            // If metadata is corrupt, leave empty
        }
    }

    return event;
}

}  // namespace mindtrace
