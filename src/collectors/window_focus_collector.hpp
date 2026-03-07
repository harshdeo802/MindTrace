#ifndef MINDTRACE_COLLECTORS_WINDOW_FOCUS_COLLECTOR_HPP
#define MINDTRACE_COLLECTORS_WINDOW_FOCUS_COLLECTOR_HPP

#include "collectors/collector_base.hpp"

namespace mindtrace {

/// Stub collector for window focus events.
class WindowFocusCollector : public CollectorBase {
public:
    WindowFocusCollector() : CollectorBase("WindowFocusCollector") {}
    void start() override;
    void stop() override;
};

}  // namespace mindtrace

#endif  // MINDTRACE_COLLECTORS_WINDOW_FOCUS_COLLECTOR_HPP
