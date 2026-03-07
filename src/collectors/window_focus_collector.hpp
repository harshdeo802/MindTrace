#ifndef MINDTRACE_COLLECTORS_WINDOW_FOCUS_COLLECTOR_HPP
#define MINDTRACE_COLLECTORS_WINDOW_FOCUS_COLLECTOR_HPP

#include "collectors/collector_base.hpp"
#include <thread>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace mindtrace {

/// Collector for window focus events.
/// Uses SetWinEventHook on Windows to track foreground window changes.
class WindowFocusCollector : public CollectorBase {
public:
    WindowFocusCollector() : CollectorBase("WindowFocusCollector") {}
    ~WindowFocusCollector() override { stop(); }

    void start() override;
    void stop() override;

private:
#ifdef _WIN32
    void run_message_loop();
    static void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, 
                                      LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
    
    std::thread thread_;
    DWORD thread_id_ = 0;
    HWINEVENTHOOK hook_ = nullptr;
    
    // Global pointer to instance for the static callback to use
    static WindowFocusCollector* instance_;
    std::string last_window_title_;
#endif
};

}  // namespace mindtrace

#endif  // MINDTRACE_COLLECTORS_WINDOW_FOCUS_COLLECTOR_HPP
