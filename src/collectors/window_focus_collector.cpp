#include "collectors/window_focus_collector.hpp"
#include <spdlog/spdlog.h>

namespace mindtrace {

void WindowFocusCollector::start() {
    if (running_.exchange(true)) return;
    spdlog::info("WindowFocusCollector started (stub mode)");
}

void WindowFocusCollector::stop() {
    if (!running_.exchange(false)) return;
    spdlog::info("WindowFocusCollector stopped");
}

}  // namespace mindtrace
