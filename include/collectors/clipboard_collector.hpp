#ifndef MINDTRACE_COLLECTORS_CLIPBOARD_COLLECTOR_HPP
#define MINDTRACE_COLLECTORS_CLIPBOARD_COLLECTOR_HPP

#include "collectors/collector_base.hpp"

namespace mindtrace {

/// Stub collector for clipboard copy events.
class ClipboardCollector : public CollectorBase {
public:
    ClipboardCollector() : CollectorBase("ClipboardCollector") {}
    void start() override;
    void stop() override;
};

}  // namespace mindtrace

#endif  // MINDTRACE_COLLECTORS_CLIPBOARD_COLLECTOR_HPP
