#include "collectors/window_focus_collector.hpp"
#include <spdlog/spdlog.h>
#include <chrono>
#include <filesystem>

#ifdef _WIN32
#include <psapi.h>
#endif

namespace mindtrace {

#ifdef _WIN32
WindowFocusCollector* WindowFocusCollector::instance_ = nullptr;

void WindowFocusCollector::start() {
    if (running_.exchange(true)) return;
    
    instance_ = this;
    
    // Start background thread to run the message loop for the hook
    thread_ = std::thread([this]() { run_message_loop(); });
    
    spdlog::info("WindowFocusCollector started (Win32)");
}

void WindowFocusCollector::stop() {
    if (!running_.exchange(false)) return;

    if (thread_.joinable()) {
        // Wait for thread to start and capture its ID
        while (thread_id_.load() == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        // Post quit message, retrying if the message queue isn't fully created yet
        while (!PostThreadMessageW(thread_id_.load(), WM_QUIT, 0, 0)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        thread_.join();
    }
    
    instance_ = nullptr;
    spdlog::info("WindowFocusCollector stopped");
}

void WindowFocusCollector::run_message_loop() {
    // Force message queue creation so PostThreadMessage won't fail
    MSG msg;
    PeekMessageW(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    
    thread_id_.store(GetCurrentThreadId());

    // Hook window focus changes (EVENT_SYSTEM_FOREGROUND)
    hook_ = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, 
        nullptr, 
        &WindowFocusCollector::WinEventProc, 
        0, 0, 
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
    );

    if (!hook_) {
        spdlog::error("WindowFocusCollector failed to set WinEventHook");
        return;
    }

    // Standard message loop to keep thread alive and receive hook callbacks
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (hook_) {
        UnhookWinEvent(hook_);
        hook_ = nullptr;
    }
}

void CALLBACK WindowFocusCollector::WinEventProc(
    HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, 
    LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) 
{
    if (event != EVENT_SYSTEM_FOREGROUND || hwnd == nullptr || instance_ == nullptr) return;
    if (!instance_->is_running()) return;

    // 1. Get window title
    const int max_title_len = 512;
    wchar_t title_buf[max_title_len] = {0};
    int len = GetWindowTextW(hwnd, title_buf, max_title_len);
    if (len == 0) return; // Window has no title or died
    
    // Narrow cast to UTF-8
    char title_utf8[max_title_len * 2] = {0};
    WideCharToMultiByte(CP_UTF8, 0, title_buf, -1, title_utf8, sizeof(title_utf8), nullptr, nullptr);
    std::string title(title_utf8);

    if (title.empty()) return; // Ignore invisible/background framework windows

    // To prevent spamming if the OS sends duplicate events
    if (title == instance_->last_window_title_) return;
    instance_->last_window_title_ = title;

    // 2. Get process executable name
    DWORD process_id = 0;
    GetWindowThreadProcessId(hwnd, &process_id);
    std::string exe_name = "Unknown";
    
    if (process_id != 0) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
        if (hProcess) {
            wchar_t path_buf[MAX_PATH] = {0};
            if (GetModuleFileNameExW(hProcess, nullptr, path_buf, MAX_PATH)) {
                std::filesystem::path full_path(path_buf);
                exe_name = full_path.filename().string();
            }
            CloseHandle(hProcess);
        }
    }

    // 3. Emit MindTrace Event
    auto now = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());

    ActivityEvent act_event;
    act_event.timestamp = now;
    act_event.type = EventType::WindowFocus;
    act_event.source = exe_name;
    act_event.content = title;

    instance_->emit(std::move(act_event));
}
#else
// Stub fallback for non-Windows platforms (already implemented/unchanged conceptually)
void WindowFocusCollector::start() {
    if (running_.exchange(true)) return;
    spdlog::info("WindowFocusCollector started (stub mode - not Windows)");
}

void WindowFocusCollector::stop() {
    if (!running_.exchange(false)) return;
    spdlog::info("WindowFocusCollector stopped");
}
#endif

}  // namespace mindtrace
