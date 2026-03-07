#include "collectors/clipboard_collector.hpp"
#include <spdlog/spdlog.h>

namespace mindtrace {

void ClipboardCollector::start() {
    if (running_.exchange(true)) return;
    spdlog::info("ClipboardCollector started (stub mode)");
}

void ClipboardCollector::stop() {
    if (!running_.exchange(false)) return;
    spdlog::info("ClipboardCollector stopped");
}

}  // namespace mindtrace
