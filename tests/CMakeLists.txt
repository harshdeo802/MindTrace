# ── Test Definitions ────────────────────────────────────────────────────

# Test: ActivityEvent model
add_executable(test_activity_event test_activity_event.cpp)
target_link_libraries(test_activity_event PRIVATE mindtrace_core GTest::gtest_main)
add_test(NAME TestActivityEvent COMMAND test_activity_event)

# Test: EventStore (SQLite)
add_executable(test_event_store test_event_store.cpp)
target_link_libraries(test_event_store PRIVATE mindtrace_core GTest::gtest_main)
add_test(NAME TestEventStore COMMAND test_event_store)

# Test: ConcurrentQueue
add_executable(test_concurrent_queue test_concurrent_queue.cpp)
target_link_libraries(test_concurrent_queue PRIVATE mindtrace_core GTest::gtest_main)
add_test(NAME TestConcurrentQueue COMMAND test_concurrent_queue)

# Test: EventPipeline
add_executable(test_event_pipeline test_event_pipeline.cpp)
target_link_libraries(test_event_pipeline PRIVATE mindtrace_core GTest::gtest_main)
add_test(NAME TestEventPipeline COMMAND test_event_pipeline)

# Test: InvertedIndex
add_executable(test_inverted_index test_inverted_index.cpp)
target_link_libraries(test_inverted_index PRIVATE mindtrace_core GTest::gtest_main)
add_test(NAME TestInvertedIndex COMMAND test_inverted_index)

# Test: QueryParser
add_executable(test_query_parser test_query_parser.cpp)
target_link_libraries(test_query_parser PRIVATE mindtrace_search GTest::gtest_main)
add_test(NAME TestQueryParser COMMAND test_query_parser)

# Test: QueryExecutor
add_executable(test_query_executor test_query_executor.cpp)
target_link_libraries(test_query_executor PRIVATE mindtrace_search GTest::gtest_main)
add_test(NAME TestQueryExecutor COMMAND test_query_executor)

# Test: TimelineBuilder
add_executable(test_timeline_builder test_timeline_builder.cpp)
target_link_libraries(test_timeline_builder PRIVATE mindtrace_timeline GTest::gtest_main)
add_test(NAME TestTimelineBuilder COMMAND test_timeline_builder)

# Test: Collectors
add_executable(test_collectors test_collectors.cpp)
target_link_libraries(test_collectors PRIVATE mindtrace_collectors GTest::gtest_main)
add_test(NAME TestCollectors COMMAND test_collectors)
