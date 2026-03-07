cmake_minimum_required(VERSION 3.20)
project(MindTrace
    VERSION 1.0.0
    DESCRIPTION "Digital Memory Search Engine"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Dependency Management via FetchContent ──────────────────────────────

include(FetchContent)

# GoogleTest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)

# spdlog (includes fmt)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    GIT_SHALLOW    TRUE
)

# nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

# Prevent GoogleTest from overriding parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest spdlog nlohmann_json)

# SQLite3 — use local amalgamation (downloaded to third_party/)
set(SQLITE3_DIR ${CMAKE_SOURCE_DIR}/third_party/sqlite3/sqlite-amalgamation-3460000)
add_library(sqlite3_lib STATIC ${SQLITE3_DIR}/sqlite3.c)
target_include_directories(sqlite3_lib PUBLIC ${SQLITE3_DIR})
if(WIN32)
    target_compile_definitions(sqlite3_lib PRIVATE SQLITE_OS_WIN=1)
endif()

# ── Core Library ────────────────────────────────────────────────────────

add_library(mindtrace_core STATIC
    src/core/event/activity_event.cpp
    src/core/storage/event_store.cpp
    src/core/pipeline/event_pipeline.cpp
    src/core/indexing/inverted_index.cpp
)

target_include_directories(mindtrace_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(mindtrace_core PUBLIC
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    sqlite3_lib
)

# ── Collectors Library ──────────────────────────────────────────────────

add_library(mindtrace_collectors STATIC
    src/collectors/filesystem_collector.cpp
    src/collectors/browser_collector.cpp
    src/collectors/window_focus_collector.cpp
    src/collectors/clipboard_collector.cpp
)

target_include_directories(mindtrace_collectors PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(mindtrace_collectors PUBLIC mindtrace_core)
if(WIN32)
    target_link_libraries(mindtrace_collectors PUBLIC psapi user32 shell32)
endif()

# ── Search Library ──────────────────────────────────────────────────────

add_library(mindtrace_search STATIC
    src/search/query_parser.cpp
    src/search/query_planner.cpp
    src/search/query_executor.cpp
)

target_include_directories(mindtrace_search PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(mindtrace_search PUBLIC mindtrace_core)

# ── Timeline Library ────────────────────────────────────────────────────

add_library(mindtrace_timeline STATIC
    src/timeline/timeline_builder.cpp
)

target_include_directories(mindtrace_timeline PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(mindtrace_timeline PUBLIC mindtrace_core)

# ── CLI Application ────────────────────────────────────────────────────

add_executable(mindtrace apps/cli/main.cpp)
target_link_libraries(mindtrace PRIVATE
    mindtrace_core
    mindtrace_collectors
    mindtrace_search
    mindtrace_timeline
)

# ── Testing ─────────────────────────────────────────────────────────────

enable_testing()
add_subdirectory(tests)
