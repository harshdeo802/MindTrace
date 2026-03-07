# MindTrace — Digital Memory Search Engine

> Index everything you do on your computer and make it searchable.

MindTrace is a local-first C++20 activity indexing engine. It captures OS events (file activity, browser history, window focus, clipboard), stores them in SQLite, and provides powerful keyword/time-based search and timeline reconstruction.

**"What was I doing last Tuesday?"** — MindTrace knows.

---

## Features

- **Activity Indexing** — File opens/edits, browser visits, window focus, clipboard
- **Natural Language Search** — `"files edited today"`, `"websites visited during work hours"`
- **Timeline Reconstruction** — Groups events into sessions by time proximity
- **Privacy-First** — All data stored locally in SQLite, never leaves your machine
- **High Performance** — In-memory inverted index, multi-threaded pipeline, sub-second search
- **Modular & Testable** — SOLID architecture, dependency injection, 50+ unit tests

---

## Architecture

```
┌──────────────┐     ┌───────────────────────────────┐     ┌────────────────┐
│  Collectors   │────>│     Event Processing Pipeline │────>│  SQLite Store  │
│  - FileSystem │     │  Normalize → Enrich → Index   │     │  + Inverted    │
│  - Browser    │     │  (Thread Pool)                │     │    Index       │
│  - Window     │     └───────────────────────────────┘     └────────┬───────┘
│  - Clipboard  │                                                    │
└──────────────┘                                                    │
                         ┌──────────────────────────────────────────┘
                         │
                    ┌────▼────┐     ┌──────────────┐     ┌──────────────┐
                    │  Query  │────>│   Timeline   │     │     CLI      │
                    │  Engine │     │   Builder    │     │  Interface   │
                    └─────────┘     └──────────────┘     └──────────────┘
```

---

## Build

### Prerequisites

- C++20 compiler (GCC 11+, Clang 14+, MSVC 19.30+)
- CMake 3.20+
- Ninja or Make

### Build Steps

```bash
mkdir build && cd build
cmake .. -G "Ninja"
cmake --build .
```

### Run Tests

```bash
cd build
ctest --output-on-failure
```

---

## Usage

```bash
# Load demo data and see example output
./mindtrace demo

# Search activity
./mindtrace search "files edited today"
./mindtrace search "websites visited this week"
./mindtrace search "What was I doing yesterday"

# View timeline
./mindtrace timeline today
./mindtrace timeline yesterday

# Database stats
./mindtrace stats
```

---

## Project Structure

```
MindTrace/
├── src/
│   ├── core/
│   │   ├── event/           ActivityEvent model + serialization
│   │   ├── pipeline/        Concurrent queue + multi-stage pipeline
│   │   ├── indexing/        Inverted index (keyword/time/app)
│   │   └── storage/         SQLite-backed EventStore
│   ├── collectors/          File, Browser, Window, Clipboard collectors
│   ├── search/              QueryParser → QueryPlanner → QueryExecutor
│   ├── timeline/            Session grouping + timeline reconstruction
│   └── common/              Logging, config, utilities
├── apps/cli/                CLI entry point
├── tests/                   50+ GoogleTest unit tests
└── CMakeLists.txt           Root build configuration
```

---

## Testing

Built with **Test Driven Development (TDD)** — every module has comprehensive tests:

| Test Suite           | Coverage                                  |
|---------------------|------------------------------------------|
| TestActivityEvent   | Construction, serialization, keywords     |
| TestEventStore      | CRUD, queries, batch insert, metadata     |
| TestConcurrentQueue | Thread safety, blocking, FIFO ordering   |
| TestEventPipeline   | Multi-stage processing, worker threads    |
| TestInvertedIndex   | Keyword/time/app search, thread safety    |
| TestQueryParser     | Natural language parsing, time resolution |
| TestQueryExecutor   | End-to-end search execution               |
| TestTimelineBuilder | Session grouping, filtering, edge cases   |
| TestCollectors      | Callback mechanism, file system scanning  |

---

## License

MIT
