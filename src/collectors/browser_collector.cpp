#include "collectors/browser_collector.hpp"
#include <spdlog/spdlog.h>

namespace mindtrace {

void BrowserCollector::start() {
    if (running_.exchange(true)) return;
    spdlog::info("BrowserCollector started (stub mode)");
}

void BrowserCollector::stop() {
    if (!running_.exchange(false)) return;
    spdlog::info("BrowserCollector stopped");
}

}  // namespace mindtrace
