#ifndef MINDTRACE_COLLECTORS_BROWSER_COLLECTOR_HPP
#define MINDTRACE_COLLECTORS_BROWSER_COLLECTOR_HPP

#include "collectors/collector_base.hpp"

namespace mindtrace {

/// Stub collector for browser history — emits mock BrowserVisit events.
class BrowserCollector : public CollectorBase {
public:
    BrowserCollector() : CollectorBase("BrowserCollector") {}
    void start() override;
    void stop() override;
};

}  // namespace mindtrace

#endif  // MINDTRACE_COLLECTORS_BROWSER_COLLECTOR_HPP
