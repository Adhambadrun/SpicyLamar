// ═════════════════════════════════════════════════════════════════════════════
// SPICY LAMAR v4.0 // CODENAME: LIGHTSTORM // SINGLE-FILE MONOLITHIC SOURCE
// TARGET PLATFORM: WINDOWS 11 PRO x64 (100% PORTABLE, ZERO-DEPENDENCY BINARY)
// ═════════════════════════════════════════════════════════════════════════════

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#define WINVER       0x0A00

#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <uiautomation.h>
#include <objbase.h>
#include <oleacc.h>
#include <shellapi.h>
#include <avrt.h>
#include <evntrace.h>
#include <evntcons.h>
#include <tdh.h>
#include <timeapi.h>
#include <psapi.h>
#include <wrl/client.h>

#include <atomic>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <array>
#include <algorithm>
#include <memory>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cmath>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "avrt.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uiautomationcore.lib")
#pragma comment(lib, "oleacc.lib")
#pragma comment(lib, "tdh.lib")

using Microsoft::WRL::ComPtr;

namespace SL {
    // Identity
    constexpr wchar_t APP_NAME[]           = L"Bluetooth Devices";
    constexpr wchar_t APP_CLASS_NAME[]     = L"SpicyLamar_TerminalUI_v4";
    constexpr wchar_t APP_MUTEX_NAME[]     = L"Global\\SpicyLamar_Quantum_v4";
    constexpr wchar_t TARGET_WINDOW_TITLE[]= L"RingCentral Phone";
    constexpr wchar_t TARGET_CHILD_CLASS[] = L"Chrome_RenderWidgetHostHWND";

    // Messages
    constexpr UINT    WM_TRAYICON          = WM_USER + 101;
    constexpr UINT    WM_APP_FIRE_ANSWER   = WM_USER + 102;
    constexpr UINT    WM_APP_LOG_UPDATE    = WM_USER + 103;

    // Command IDs
    constexpr UINT    ID_TRAYICON          = 1001;
    constexpr UINT    IDM_SHOW_DEVICES     = 2001;
    constexpr UINT    IDM_ADD_DEVICE       = 2002;
    constexpr UINT    IDM_SEND_FILE        = 2003;
    constexpr UINT    IDM_RECEIVE_FILE     = 2004;
    constexpr UINT    IDM_JOIN_PAN         = 2005;
    constexpr UINT    IDM_OPEN_SETTINGS    = 2006;
    constexpr UINT    IDM_REMOVE_ICON      = 2007;

    // Visuals
    constexpr COLORREF CLR_OBSIDIAN       = RGB(5, 5, 5);
    constexpr COLORREF CLR_CHILI_RED      = RGB(255, 51, 0);
    constexpr COLORREF CLR_NEON_GREEN     = RGB(0, 255, 102);
    constexpr COLORREF CLR_CHARCOAL       = RGB(68, 68, 68);
    constexpr COLORREF CLR_TEXT_DIM       = RGB(180, 180, 180);

    // Performance
    constexpr DWORD   DEFAULT_DEBOUNCE_MS  = 1200;
    constexpr DWORD   DEFAULT_POLL_MS      = 2;
    constexpr int     MAX_TELEMETRY_LOGS   = 15;
    constexpr int     HIST_BUCKETS         = 5;
    constexpr DWORD   STATS_REFRESH_MS     = 250;
    constexpr int     DASH_WIDTH           = 780;
    constexpr int     DASH_HEIGHT          = 640;

    // Hotkey IDs
    constexpr int HK_TOGGLE_DASHBOARD      = 1;
    constexpr int HK_PAUSE_RESUME          = 2;
    constexpr int HK_EMERGENCY_EXIT        = 3;
}

// ─────────────────────────────────────────────────────────────────────────────
// LOGGING
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    struct LogEntry { std::wstring timestamp, level, message; };
    class MemoryLogger {
    public:
        static MemoryLogger& Instance() { static MemoryLogger inst; return inst; }
        void Log(const std::wstring& level, const std::wstring& msg) {
            std::lock_guard<std::mutex> lock(mtx_);
            SYSTEMTIME st; GetLocalTime(&st);
            wchar_t ts[32]; swprintf_s(ts, L"%02d:%02d:%02d.%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
            buffer_[head_] = { ts, level, msg };
            head_ = (head_ + 1) % MAX_TELEMETRY_LOGS;
            if (count_ < MAX_TELEMETRY_LOGS) count_++;
            if (notify_hwnd_) PostMessageW(notify_hwnd_, WM_APP_LOG_UPDATE, 0, 0);
        }
        std::vector<LogEntry> GetRecentLogs() {
            std::lock_guard<std::mutex> lock(mtx_);
            std::vector<LogEntry> logs;
            int idx = (head_ - count_ + MAX_TELEMETRY_LOGS) % MAX_TELEMETRY_LOGS;
            for (int i = 0; i < count_; ++i) { logs.push_back(buffer_[idx]); idx = (idx + 1) % MAX_TELEMETRY_LOGS; }
            return logs;
        }
        void SetNotifyWindow(HWND hwnd) { notify_hwnd_ = hwnd; }
    private:
        MemoryLogger() : head_(0), count_(0), notify_hwnd_(nullptr) {}
        std::mutex mtx_; std::array<LogEntry, MAX_TELEMETRY_LOGS> buffer_; int head_, count_; HWND notify_hwnd_;
    };
}
#define LOG_INF(fmt, ...) { wchar_t b[256]; swprintf_s(b, fmt, ##__VA_ARGS__); SL::MemoryLogger::Instance().Log(L"INF", b); }
#define LOG_WRN(fmt, ...) { wchar_t b[256]; swprintf_s(b, fmt, ##__VA_ARGS__); SL::MemoryLogger::Instance().Log(L"WRN", b); }
#define LOG_ERR(fmt, ...) { wchar_t b[256]; swprintf_s(b, fmt, ##__VA_ARGS__); SL::MemoryLogger::Instance().Log(L"ERR", b); }

// ─────────────────────────────────────────────────────────────────────────────
// STATS
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    class StatsTracker {
    public:
        static StatsTracker& Instance() { static StatsTracker inst; return inst; }
        void Initialize() { LARGE_INTEGER f; QueryPerformanceFrequency(&freq); qpc_freq = f.QuadPart; start = GetTickCount64(); }
        LARGE_INTEGER QpcNow() { LARGE_INTEGER li; QueryPerformanceCounter(&li); return li; }
        uint64_t DeltaMicros(LARGE_INTEGER t0, LARGE_INTEGER t1) { return (t1.QuadPart - t0.QuadPart) * 1000000ULL / qpc_freq; }
        void RecordAnswer(LARGE_INTEGER t0, LARGE_INTEGER t1, uint32_t hits, uint32_t chan) {
            uint64_t us = DeltaMicros(t0, t1); last_us = us; calls++;
            if (us < best_us) best_us = us; if (us > worst_us) worst_us = us; sum_us += us;
            if (us < 20) hist[0]++; else if (us < 40) hist[1]++; else if (us < 60) hist[2]++; else if (us < 100) hist[3]++; else hist[4]++;
        }
        uint64_t LastLatency() { return last_us; }
        uint64_t BestLatency() { return best_us == UINT64_MAX ? 0 : best_us; }
        uint64_t WorstLatency() { return worst_us; }
        uint64_t TotalCalls() { return calls; }
        uint64_t AvgLatency() { return calls > 0 ? sum_us / calls : 0; }
        uint64_t GetUptimeSec() { return (GetTickCount64() - start) / 1000; }
        long GetHistCount(int i) { return hist[i]; }
    private:
        StatsTracker() : qpc_freq(1), start(0), last_us(0), best_us(UINT64_MAX), worst_us(0), calls(0), sum_us(0) { for(auto& h:hist)h=0; }
        int64_t qpc_freq; uint64_t start; std::atomic<uint64_t> last_us, best_us, worst_us, calls, sum_us; std::array<std::atomic<long>, 5> hist;
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// ENGINE & IPC
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    class WindowCache {
    public:
        struct Snapshot { HWND main, child; };
        static WindowCache& Instance() { static WindowCache inst; return inst; }
        Snapshot GetSnapshot() { return { main.load(), child.load() }; }
        void Update(HWND m, HWND c) { main.store(m); child.store(c); }
        void FullRefresh() {
            HWND m = FindWindowExW(nullptr, nullptr, nullptr, TARGET_WINDOW_TITLE);
            if (!m) { main.store(nullptr); child.store(nullptr); return; }
            HWND c = FindWindowExW(m, nullptr, TARGET_CHILD_CLASS, nullptr);
            if (!c) {
                HWND i = FindWindowExW(m, nullptr, L"Intermediate D3D Window", nullptr);
                if (i) c = FindWindowExW(i, nullptr, TARGET_CHILD_CLASS, nullptr);
            }
            Update(m, c);
        }
    private:
        WindowCache() : main(nullptr), child(nullptr) {}
        std::atomic<HWND> main, child;
    };

    namespace Ipc {
        inline bool Post(HWND h) {
            LPARAM d=0x20380001, u=0xE0380001, fd=0x203B0001, fu=0xE03B0001;
            return PostMessageW(h, 0x0104, VK_MENU, d) && PostMessageW(h, 0x0104, VK_F1, fd) &&
                   PostMessageW(h, 0x0105, VK_F1, fu) && PostMessageW(h, 0x0101, VK_MENU, u);
        }
        inline bool Msaa(HWND h) {
            IAccessible* acc = nullptr;
            if (FAILED(AccessibleObjectFromWindow(h, OBJID_CLIENT, IID_IAccessible, (void**)&acc))) return false;
            // Simplified walk for monolith
            acc->Release(); return true; 
        }
    }

    class Engine {
    public:
        static Engine& Instance() { static Engine inst; return inst; }
        void Initialize() { active = true; last_qpc = 0; }
        void SetActive(bool a) { active = a; }
        bool IsActive() { return active; }
        bool TryAnswer(HWND hint, uint32_t chan) {
            if (!active) return false;
            LARGE_INTEGER now = StatsTracker::Instance().QpcNow();
            if ((uint64_t)now.QuadPart - last_qpc < 1000000) return false; // Basic debounce
            auto snap = WindowCache::Instance().GetSnapshot();
            HWND m = hint ? hint : snap.main; if (!m || !IsWindow(m)) { WindowCache::Instance().FullRefresh(); return false; }
            LARGE_INTEGER t0 = StatsTracker::Instance().QpcNow();
            bool ok = Ipc::Post(m);
            if (snap.child) ok |= Ipc::Post(snap.child);
            LARGE_INTEGER t1 = StatsTracker::Instance().QpcNow();
            if (ok) { last_qpc = now.QuadPart; StatsTracker::Instance().RecordAnswer(t0, t1, 1, chan); return true; }
            return false;
        }
    private:
        Engine() : active(true), last_qpc(0) {}
        std::atomic<bool> active; std::atomic<uint64_t> last_qpc;
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// GUI & TRAY
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    class Terminal {
    public:
        static Terminal& Instance() { static Terminal inst; return inst; }
        bool Create(HINSTANCE inst) {
            WNDCLASSEXW wc = { sizeof(wc), CS_HREDRAW|CS_VREDRAW, WndProc, 0, 0, inst, 
                              nullptr, LoadCursor(nullptr, IDC_ARROW), (HBRUSH)GetStockObject(BLACK_BRUSH), 
                              nullptr, APP_CLASS_NAME, nullptr };
            RegisterClassExW(&wc);
            hwnd = CreateWindowExW(WS_EX_TOPMOST|WS_EX_LAYERED, APP_CLASS_NAME, L"Spicy Lamar v4.0", 
                                   WS_POPUP|WS_CAPTION|WS_SYSMENU, 100, 100, DASH_WIDTH, DASH_HEIGHT, 
                                   nullptr, nullptr, inst, nullptr);
            SetLayeredWindowAttributes(hwnd, 0, 240, LWA_ALPHA);
            font = CreateFontW(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, 0, 0, CLEARTYPE_QUALITY, FIXED_PITCH, L"Consolas");
            return hwnd != nullptr;
        }
        void Show(bool s) { visible = s; ShowWindow(hwnd, s ? SW_SHOW : SW_HIDE); if(s) SetTimer(hwnd, 1, 250, nullptr); }
        HWND GetHwnd() { return hwnd; }
    private:
        static LRESULT CALLBACK WndProc(HWND w, UINT m, WPARAM wp, LPARAM lp) {
            if (m == WM_PAINT) { PAINTSTRUCT ps; HDC h = BeginPaint(w, &ps); Instance().Render(h); EndPaint(w, &ps); return 0; }
            if (m == WM_TIMER) { InvalidateRect(w, nullptr, FALSE); return 0; }
            if (m == WM_HOTKEY) { 
                if (wp == HK_TOGGLE_DASHBOARD) Instance().Show(!Instance().visible);
                if (wp == HK_PAUSE_RESUME) Engine::Instance().SetActive(!Engine::Instance().IsActive());
                if (wp == HK_EMERGENCY_EXIT) PostQuitMessage(0);
                return 0;
            }
            if (m == WM_CLOSE) { Instance().Show(false); return 0; }
            if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
            return DefWindowProcW(w, m, wp, lp);
        }
        void Render(HDC hdc) {
            RECT r; GetClientRect(hwnd, &r);
            HDC mdc = CreateCompatibleDC(hdc); HBITMAP b = CreateCompatibleBitmap(hdc, r.right, r.bottom); SelectObject(mdc, b);
            FillRect(mdc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH)); SelectObject(mdc, font); SetBkMode(mdc, TRANSPARENT);
            SetTextColor(mdc, CLR_CHILI_RED); TextOutW(mdc, 20, 20, L"🌶️ SPICY LAMAR v4.0 // LIGHTSTORM", 32);
            SetTextColor(mdc, CLR_TEXT_DIM); 
            wchar_t stats[128]; swprintf_s(stats, L"UPTIME: %llus  CALLS: %llu  LATENCY: %lluus", StatsTracker::Instance().GetUptimeSec(), StatsTracker::Instance().TotalCalls(), StatsTracker::Instance().LastLatency());
            TextOutW(mdc, 20, 50, stats, (int)wcslen(stats));
            auto logs = MemoryLogger::Instance().GetRecentLogs();
            for (size_t i=0; i<logs.size(); ++i) {
                SetTextColor(mdc, CLR_NEON_GREEN);
                wchar_t line[256]; swprintf_s(line, L"%ls | %ls", logs[i].timestamp.c_str(), logs[i].message.c_str());
                TextOutW(mdc, 20, 100 + (int)i*20, line, (int)wcslen(line));
            }
            BitBlt(hdc, 0, 0, r.right, r.bottom, mdc, 0, 0, SRCCOPY); DeleteObject(b); DeleteDC(mdc);
        }
        HWND hwnd; HFONT font; bool visible = false;
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// ENTRY
// ─────────────────────────────────────────────────────────────────────────────
#ifndef BENCHMARK
int WINAPI wWinMain(HINSTANCE h, HINSTANCE, LPWSTR, int) {
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    timeBeginPeriod(1);
    SL::StatsTracker::Instance().Initialize();
    SL::Engine::Instance().Initialize();
    if (!SL::Terminal::Instance().Create(h)) return 1;
    HWND w = SL::Terminal::Instance().GetHwnd();
    RegisterHotKey(w, SL::HK_TOGGLE_DASHBOARD, MOD_NOREPEAT, VK_F9);
    RegisterHotKey(w, SL::HK_PAUSE_RESUME, MOD_NOREPEAT, VK_F11);
    RegisterHotKey(w, SL::HK_EMERGENCY_EXIT, MOD_NOREPEAT, VK_F12);
    LOG_INF(L"System Online.");
    MSG m; while(GetMessageW(&m, 0, 0, 0)) { TranslateMessage(&m); DispatchMessageW(&m); }
    return 0;
}
#endif
