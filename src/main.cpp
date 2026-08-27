// ═════════════════════════════════════════════════════════════════════════════
// SPICY LAMAR v4.0 // RINGCENTRAL AUTO-ANSWER // SINGLE-FILE MONOLITHIC SOURCE
// TARGET PLATFORM: WINDOWS 10/11 x64 (PORTABLE, STATICALLY LINKED)
// ═════════════════════════════════════════════════════════════════════════════

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

// Guarded so these can also be supplied on the compiler command line
// (e.g. -DWIN32_LEAN_AND_MEAN -DNOMINMAX) without C4005 redefinition warnings.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#define WINVER       0x0A00
#endif

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
#pragma comment(lib, "psapi.lib")

using Microsoft::WRL::ComPtr;

namespace SL {
    // Identity Configuration
    constexpr wchar_t APP_NAME[]           = L"Spicy Lamar // RingCentral Auto-Answer";
    constexpr wchar_t APP_CLASS_NAME[]     = L"SpicyLamar_AutoAnswer_v4";
    constexpr wchar_t APP_MUTEX_NAME[]     = L"Global\\SpicyLamar_AutoAnswer_v4";
    constexpr wchar_t TARGET_WINDOW_TITLE[]= L"RingCentral Phone";
    constexpr wchar_t TARGET_CHILD_CLASS[] = L"Chrome_RenderWidgetHostHWND";

    // Windows Messages
    constexpr UINT    WM_TRAYICON          = WM_USER + 101;
    constexpr UINT    WM_APP_FIRE_ANSWER   = WM_USER + 102;
    constexpr UINT    WM_APP_LOG_UPDATE    = WM_USER + 103;

    // Command & Menu IDs
    constexpr UINT    ID_TRAYICON          = 1001;
    constexpr UINT    IDM_SHOW_DEVICES     = 2001;
    constexpr UINT    IDM_ADD_DEVICE       = 2002;
    constexpr UINT    IDM_SEND_FILE        = 2003;
    constexpr UINT    IDM_RECEIVE_FILE     = 2004;
    constexpr UINT    IDM_JOIN_PAN         = 2005;
    constexpr UINT    IDM_OPEN_SETTINGS    = 2006;
    constexpr UINT    IDM_REMOVE_ICON      = 2007;

    // Visual Palette (Obsidian & Chili Neon)
    constexpr COLORREF CLR_OBSIDIAN       = RGB(5, 5, 5);
    constexpr COLORREF CLR_CHILI_RED      = RGB(255, 51, 0);
    constexpr COLORREF CLR_NEON_GREEN     = RGB(0, 255, 102);
    constexpr COLORREF CLR_CHARCOAL       = RGB(68, 68, 68);
    constexpr COLORREF CLR_TEXT_DIM       = RGB(180, 180, 180);

    // Performance & Telemetry Constants
    constexpr DWORD   DEFAULT_DEBOUNCE_MS  = 1200;
    constexpr DWORD   DEFAULT_POLL_MS      = 20;
    constexpr int     MAX_TELEMETRY_LOGS   = 15;
    constexpr int     HIST_BUCKETS         = 5;
    constexpr DWORD   STATS_REFRESH_MS     = 250;
    constexpr int     DASH_WIDTH           = 780;
    constexpr int     DASH_HEIGHT          = 520;

    // Global Hotkey Identifiers
    constexpr int HK_TOGGLE_DASHBOARD      = 1;
    constexpr int HK_PAUSE_RESUME          = 2;
    constexpr int HK_EMERGENCY_EXIT        = 3;
}

// ─────────────────────────────────────────────────────────────────────────────
// LOGGING SUBSYSTEM
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    struct LogEntry { 
        std::wstring timestamp; 
        std::wstring level; 
        std::wstring message; 
    };

    class MemoryLogger {
    public:
        static MemoryLogger& Instance() { 
            static MemoryLogger inst; 
            return inst; 
        }

        void Log(const std::wstring& level, const std::wstring& msg) {
            std::lock_guard<std::mutex> lock(mtx_);
            SYSTEMTIME st; 
            GetLocalTime(&st);
            wchar_t ts[32]; 
            swprintf_s(ts, L"%02d:%02d:%02d.%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
            buffer_[head_] = { ts, level, msg };
            head_ = (head_ + 1) % MAX_TELEMETRY_LOGS;
            if (count_ < MAX_TELEMETRY_LOGS) count_++;
            if (notify_hwnd_ && IsWindow(notify_hwnd_)) {
                PostMessageW(notify_hwnd_, WM_APP_LOG_UPDATE, 0, 0);
            }
        }

        std::vector<LogEntry> GetRecentLogs() {
            std::lock_guard<std::mutex> lock(mtx_);
            std::vector<LogEntry> logs;
            logs.reserve(count_);
            int idx = (head_ - count_ + MAX_TELEMETRY_LOGS) % MAX_TELEMETRY_LOGS;
            for (int i = 0; i < count_; ++i) { 
                logs.push_back(buffer_[idx]); 
                idx = (idx + 1) % MAX_TELEMETRY_LOGS; 
            }
            return logs;
        }

        void SetNotifyWindow(HWND hwnd) { notify_hwnd_ = hwnd; }

    private:
        MemoryLogger() : head_(0), count_(0), notify_hwnd_(nullptr) {}
        std::mutex mtx_; 
        std::array<LogEntry, MAX_TELEMETRY_LOGS> buffer_; 
        int head_, count_; 
        HWND notify_hwnd_;
    };
}

#define LOG_INF(fmt, ...) { wchar_t b[256]; swprintf_s(b, fmt, ##__VA_ARGS__); SL::MemoryLogger::Instance().Log(L"INF", b); }
#define LOG_WRN(fmt, ...) { wchar_t b[256]; swprintf_s(b, fmt, ##__VA_ARGS__); SL::MemoryLogger::Instance().Log(L"WRN", b); }
#define LOG_ERR(fmt, ...) { wchar_t b[256]; swprintf_s(b, fmt, ##__VA_ARGS__); SL::MemoryLogger::Instance().Log(L"ERR", b); }

// ─────────────────────────────────────────────────────────────────────────────
// STATS & TELEMETRY TRACKER
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    class StatsTracker {
    public:
        static StatsTracker& Instance() { 
            static StatsTracker inst; 
            return inst; 
        }

        void Initialize() { 
            LARGE_INTEGER f; 
            QueryPerformanceFrequency(&f); 
            qpc_freq = f.QuadPart; 
            start = GetTickCount64(); 
        }

        LARGE_INTEGER QpcNow() { 
            LARGE_INTEGER li; 
            QueryPerformanceCounter(&li); 
            return li; 
        }

        uint64_t DeltaMicros(LARGE_INTEGER t0, LARGE_INTEGER t1) { 
            if (qpc_freq <= 0) return 0;
            return (uint64_t)((t1.QuadPart - t0.QuadPart) * 1000000ULL / qpc_freq); 
        }

        void RecordAnswer(LARGE_INTEGER t0, LARGE_INTEGER t1, uint32_t hits, uint32_t chan) {
            uint64_t us = DeltaMicros(t0, t1); 
            last_us = us; 
            calls++;
            if (us < best_us) best_us = us; 
            if (us > worst_us) worst_us = us; 
            sum_us += us;

            if (us < 20) hist[0]++; 
            else if (us < 40) hist[1]++; 
            else if (us < 60) hist[2]++; 
            else if (us < 100) hist[3]++; 
            else hist[4]++;
        }

        uint64_t LastLatency() const { return last_us.load(); }
        uint64_t BestLatency() const { return best_us.load() == UINT64_MAX ? 0 : best_us.load(); }
        uint64_t WorstLatency() const { return worst_us.load(); }
        uint64_t TotalCalls() const { return calls.load(); }
        uint64_t AvgLatency() const { 
            uint64_t c = calls.load(); 
            return c > 0 ? (sum_us.load() / c) : 0; 
        }
        uint64_t GetUptimeSec() const { return (GetTickCount64() - start) / 1000; }
        long GetHistCount(int i) const { 
            if (i >= 0 && i < HIST_BUCKETS) return hist[i].load(); 
            return 0; 
        }

    private:
        StatsTracker() 
            : qpc_freq(1), start(0), last_us(0), best_us(UINT64_MAX), worst_us(0), calls(0), sum_us(0) { 
            for (auto& h : hist) h = 0; 
        }

        int64_t qpc_freq; 
        uint64_t start; 
        std::atomic<uint64_t> last_us, best_us, worst_us, calls, sum_us; 
        std::array<std::atomic<long>, HIST_BUCKETS> hist;
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// WINDOW CACHE & ENUMERATION
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    class WindowCache {
    public:
        struct Snapshot { HWND main, child; };

        static WindowCache& Instance() { 
            static WindowCache inst; 
            return inst; 
        }

        Snapshot GetSnapshot() { 
            return { main.load(), child.load() }; 
        }

        void Update(HWND m, HWND c) { 
            main.store(m); 
            child.store(c); 
        }

        HWND FindRingCentral() {
            struct SearchData {
                HWND found;
                const wchar_t* query;
            } data = { nullptr, TARGET_WINDOW_TITLE };

            EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
                auto* d = reinterpret_cast<SearchData*>(lp);
                if (!IsWindowVisible(hwnd)) return TRUE;

                wchar_t title[256] = {0};
                GetWindowTextW(hwnd, title, 256);
                if (wcsstr(title, d->query) != nullptr) {
                    d->found = hwnd;
                    return FALSE; // Stop enumeration once found
                }
                return TRUE;
            }, reinterpret_cast<LPARAM>(&data));

            if (data.found) {
                HWND c = FindWindowExW(data.found, nullptr, TARGET_CHILD_CLASS, nullptr);
                if (!c) {
                    HWND intermediate = FindWindowExW(data.found, nullptr, L"Intermediate D3D Window", nullptr);
                    if (intermediate) c = FindWindowExW(intermediate, nullptr, TARGET_CHILD_CLASS, nullptr);
                }
                Update(data.found, c);
            }
            return data.found;
        }

    private:
        WindowCache() : main(nullptr), child(nullptr) {}
        std::atomic<HWND> main, child;
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// ANSWER ENGINE (7-SHOT IPC CASCADE)
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    class Engine {
    public:
        static Engine& Instance() { 
            static Engine inst; 
            return inst; 
        }

        void Initialize() { 
            active = true; 
            last_fire_tick = 0; 
        }

        void SetActive(bool a) { active = a; }
        bool IsActive() const { return active; }

        bool TryAnswer(HWND hint, uint32_t chan, bool bypassDebounce = false) {
            if (!active) return false;

#ifdef BENCHMARK
            bypassDebounce = true;
#endif

            LARGE_INTEGER now = StatsTracker::Instance().QpcNow();
            if (!bypassDebounce && last_fire_tick.load() != 0) {
                LARGE_INTEGER prev;
                prev.QuadPart = (LONGLONG)last_fire_tick.load();
                uint64_t elapsed_us = StatsTracker::Instance().DeltaMicros(prev, now);
                if (elapsed_us < ((uint64_t)DEFAULT_DEBOUNCE_MS * 1000ULL)) {
                    return false; // Debounce active
                }
            }

            HWND m = hint;
            if (!m || !IsWindow(m)) {
                m = WindowCache::Instance().FindRingCentral();
            }
            if (!m || !IsWindow(m)) return false;

            LARGE_INTEGER t0 = StatsTracker::Instance().QpcNow();

            auto snap = WindowCache::Instance().GetSnapshot();
            HWND child = snap.child;
            if (!child || !IsWindow(child)) {
                child = FindWindowExW(m, nullptr, TARGET_CHILD_CLASS, nullptr);
            }

            // ─────────────────────────────────────────────────────────────────
            // 7-SHOT REDUNDANT IPC CASCADE
            // ─────────────────────────────────────────────────────────────────
            
            // Shot 1: Target window Alt+F1 Down/Up
            PostMessageW(m, WM_SYSKEYDOWN, VK_MENU, 0x20380001);
            PostMessageW(m, WM_SYSKEYDOWN, VK_F1,   0x203B0001);
            PostMessageW(m, WM_SYSKEYUP,   VK_F1,   0xE03B0001);
            PostMessageW(m, WM_KEYUP,      VK_MENU, 0xE0380001);

            // Shot 2: Chrome Render Child Alt+F1 Down/Up
            if (child && IsWindow(child)) {
                PostMessageW(child, WM_SYSKEYDOWN, VK_MENU, 0x20380001);
                PostMessageW(child, WM_SYSKEYDOWN, VK_F1,   0x203B0001);
                PostMessageW(child, WM_SYSKEYUP,   VK_F1,   0xE03B0001);
                PostMessageW(child, WM_KEYUP,      VK_MENU, 0xE0380001);
            }

            // Shot 3: Post Enter & Space keys to root
            PostMessageW(m, WM_KEYDOWN, VK_RETURN, 0x001C0001);
            PostMessageW(m, WM_KEYUP,   VK_RETURN, 0xC01C0001);

            // Shot 4: Simulated hardware key cascade (keybd_event)
            keybd_event(VK_MENU, 0x38, 0, 0);
            keybd_event(VK_F1,   0x3B, 0, 0);
            keybd_event(VK_F1,   0x3B, KEYEVENTF_KEYUP, 0);
            keybd_event(VK_MENU, 0x38, KEYEVENTF_KEYUP, 0);

            // Shot 5: Direct RingCentral Command message
            PostMessageW(m, WM_COMMAND, MAKEWPARAM(1001, 0), 0);

            // Shot 6: Foreground focus activation
            SetForegroundWindow(m);

            // Shot 7: Modifier key release safety net
            keybd_event(VK_MENU,    0x38, KEYEVENTF_KEYUP, 0);
            keybd_event(VK_CONTROL, 0x1D, KEYEVENTF_KEYUP, 0);

            LARGE_INTEGER t1 = StatsTracker::Instance().QpcNow();
            last_fire_tick.store((uint64_t)now.QuadPart);
            StatsTracker::Instance().RecordAnswer(t0, t1, 7, chan);

            uint64_t lat = StatsTracker::Instance().DeltaMicros(t0, t1);
            LOG_INF(L"ANSWERED via 7-Shot Cascade [Chan: %u] in %lluus", chan, lat);
            return true;
        }

    private:
        Engine() : active(true), last_fire_tick(0) {}
        std::atomic<bool> active; 
        std::atomic<uint64_t> last_fire_tick;
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// DETECTION HOOKS (WINEVENT + SHELLHOOK)
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    inline void CALLBACK GlobalWinEventProc(
        HWINEVENTHOOK hWinEventHook, 
        DWORD event, 
        HWND hwnd, 
        LONG idObject, 
        LONG idChild, 
        DWORD idEventThread, 
        DWORD dwmsEventTime) 
    {
        if (idObject != OBJID_WINDOW || hwnd == nullptr) return;
        if (event == EVENT_OBJECT_DESTROY) return;

        HWND root = GetAncestor(hwnd, GA_ROOT);
        if (!root) root = hwnd;

        wchar_t title[256] = {0};
        GetWindowTextW(root, title, 256);
        if (wcsstr(title, TARGET_WINDOW_TITLE) != nullptr) {
            Engine::Instance().TryAnswer(root, 1);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TERMINAL DASHBOARD & TRAY
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    class Terminal {
    public:
        static Terminal& Instance() { 
            static Terminal inst; 
            return inst; 
        }

        bool Create(HINSTANCE inst) {
            hInst_ = inst;

            WNDCLASSEXW wc = { 
                sizeof(wc), 
                CS_HREDRAW | CS_VREDRAW, 
                WndProc, 
                0, 0, 
                inst, 
                LoadAppIcon(), 
                LoadCursor(nullptr, IDC_ARROW), 
                (HBRUSH)GetStockObject(BLACK_BRUSH), 
                nullptr, 
                APP_CLASS_NAME, 
                nullptr 
            };
            RegisterClassExW(&wc);

            hwnd = CreateWindowExW(
                WS_EX_TOPMOST, 
                APP_CLASS_NAME, 
                L"Spicy Lamar v4.0 // RingCentral Auto-Answer", 
                WS_POPUP | WS_CAPTION | WS_SYSMENU, 
                100, 100, 
                DASH_WIDTH, DASH_HEIGHT, 
                nullptr, nullptr, inst, nullptr
            );

            if (!hwnd) return false;

            // Obsidian terminal font
            font = CreateFontW(
                14, 0, 0, 0, FW_NORMAL, 0, 0, 0, 
                ANSI_CHARSET, 0, 0, 
                CLEARTYPE_QUALITY, FIXED_PITCH, 
                L"Consolas"
            );

            // Initialize tray icon
            InitializeTray();

            // Register 5-Channel detection hooks
            hook_ = SetWinEventHook(
                0x0003, 0x800C, 
                nullptr, 
                GlobalWinEventProc, 
                0, 0, 
                WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
            );

            wm_shellhook_ = RegisterWindowMessageW(L"SHELLHOOK");
            RegisterShellHookWindow(hwnd);

            // High-resolution polling timer (20ms) and UI refresh timer (250ms)
            SetTimer(hwnd, 1, STATS_REFRESH_MS, nullptr);
            SetTimer(hwnd, 2, DEFAULT_POLL_MS, nullptr);

            MemoryLogger::Instance().SetNotifyWindow(hwnd);
            return true;
        }

        void Show(bool s) { 
            visible = s; 
            ShowWindow(hwnd, s ? SW_SHOW : SW_HIDE); 
            if (s) {
                SetForegroundWindow(hwnd);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }

        bool IsVisible() const { return visible; }
        HWND GetHwnd() const { return hwnd; }

    private:
        HICON LoadAppIcon() {
            HICON h = LoadIconW(hInst_, MAKEINTRESOURCEW(1));
            if (h) return h;
            return LoadIconW(nullptr, IDI_APPLICATION);
        }

        void InitializeTray() {
            ZeroMemory(&nid_, sizeof(nid_));
            nid_.cbSize = sizeof(nid_);
            nid_.hWnd = hwnd;
            nid_.uID = ID_TRAYICON;
            nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            nid_.uCallbackMessage = WM_TRAYICON;
            nid_.hIcon = LoadAppIcon();
            wcscpy_s(nid_.szTip, APP_NAME);

            Shell_NotifyIconW(NIM_ADD, &nid_);
        }

        void ShowTrayMenu() {
            HMENU hMenu = CreatePopupMenu();
            InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, IDM_OPEN_SETTINGS, L"Open Dashboard");
            InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_STRING, IDM_ADD_DEVICE, L"Pause/Resume");
            InsertMenuW(hMenu, 2, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
            InsertMenuW(hMenu, 3, MF_BYPOSITION | MF_STRING, IDM_REMOVE_ICON, L"Exit");

            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(hMenu);
        }

        static LRESULT CALLBACK WndProc(HWND w, UINT m, WPARAM wp, LPARAM lp) {
            Terminal& self = Instance();

            if (m == WM_PAINT) { 
                PAINTSTRUCT ps; 
                HDC h = BeginPaint(w, &ps); 
                self.Render(h); 
                EndPaint(w, &ps); 
                return 0; 
            }

            if (m == WM_TIMER) { 
                if (wp == 1) {
                    if (self.visible) InvalidateRect(w, nullptr, FALSE);
                } else if (wp == 2) {
                    HWND found = WindowCache::Instance().FindRingCentral();
                    if (found && IsWindowVisible(found)) {
                        Engine::Instance().TryAnswer(found, 2);
                    }
                }
                return 0; 
            }

            if (m == WM_TRAYICON) {
                if (lp == WM_RBUTTONUP || lp == WM_CONTEXTMENU) {
                    self.ShowTrayMenu();
                } else if (lp == WM_LBUTTONDBLCLK) {
                    self.Show(!self.visible);
                }
                return 0;
            }

            if (m == WM_COMMAND) {
                UINT id = LOWORD(wp);
                switch (id) {
                    case IDM_ADD_DEVICE:
                        Engine::Instance().SetActive(!Engine::Instance().IsActive());
                        break;
                    case IDM_OPEN_SETTINGS:
                        self.Show(!self.visible);
                        break;
                    case IDM_REMOVE_ICON:
                        PostQuitMessage(0);
                        break;
                    default:
                        break;
                }
                return 0;
            }

            if (m == WM_HOTKEY) { 
                if (wp == HK_TOGGLE_DASHBOARD) self.Show(!self.visible);
                if (wp == HK_PAUSE_RESUME) Engine::Instance().SetActive(!Engine::Instance().IsActive());
                if (wp == HK_EMERGENCY_EXIT) PostQuitMessage(0);
                return 0;
            }

            if (m == self.wm_shellhook_) {
                if (wp == HSHELL_WINDOWCREATED || wp == HSHELL_RUDEAPPACTIVATED || wp == HSHELL_FLASH) {
                    HWND candidate = (HWND)lp;
                    wchar_t title[256] = {0};
                    GetWindowTextW(candidate, title, 256);
                    if (wcsstr(title, TARGET_WINDOW_TITLE) != nullptr) {
                        Engine::Instance().TryAnswer(candidate, 3);
                    }
                }
                return 0;
            }

            if (m == WM_APP_LOG_UPDATE) {
                if (self.visible) InvalidateRect(w, nullptr, FALSE);
                return 0;
            }

            if (m == WM_SYSCOMMAND) {
                if ((wp & 0xFFF0) == SC_CLOSE) {
                    // Hide to tray instead of exiting when user closes via Alt+F4 / X button
                    self.Show(false);
                    return 0;
                }
            }

            if (m == WM_CLOSE) { 
                self.Show(false); 
                return 0; 
            }

            if (m == WM_DESTROY) { 
                Shell_NotifyIconW(NIM_DELETE, &self.nid_);
                if (self.hook_) UnhookWinEvent(self.hook_);
                DeregisterShellHookWindow(w);
                PostQuitMessage(0); 
                return 0; 
            }

            return DefWindowProcW(w, m, wp, lp);
        }

        void Render(HDC hdc) {
            RECT r; 
            GetClientRect(hwnd, &r);

            HDC mdc = CreateCompatibleDC(hdc); 
            HBITMAP b = CreateCompatibleBitmap(hdc, r.right, r.bottom); 
            HGDIOBJ oldBmp = SelectObject(mdc, b);
            HGDIOBJ oldFont = SelectObject(mdc, font);

            // Dark canvas background
            HBRUSH bgBrush = CreateSolidBrush(CLR_OBSIDIAN);
            FillRect(mdc, &r, bgBrush);
            DeleteObject(bgBrush);

            SetBkMode(mdc, TRANSPARENT);

            // Header separator line
            HPEN linePen = CreatePen(PS_SOLID, 1, CLR_CHARCOAL);
            HGDIOBJ oldPen = SelectObject(mdc, linePen);
            MoveToEx(mdc, 20, 45, nullptr);
            LineTo(mdc, DASH_WIDTH - 20, 45);

            // Title Banner
            SetTextColor(mdc, CLR_CHILI_RED);
            const wchar_t* titleText = L"🌶️ SPICY LAMAR v4.0 // RINGCENTRAL AUTO-ANSWER";
            TextOutW(mdc, 20, 20, titleText, (int)wcslen(titleText));

            // Status Badge
            SetTextColor(mdc, Engine::Instance().IsActive() ? CLR_NEON_GREEN : CLR_CHILI_RED);
            const wchar_t* statusStr = Engine::Instance().IsActive() ? L"STATUS: [🌶️ ACTIVE]" : L"STATUS: [⚠ PAUSED]";
            TextOutW(mdc, 20, 55, statusStr, (int)wcslen(statusStr));

            // Metric Counters
            SetTextColor(mdc, CLR_TEXT_DIM); 
            wchar_t stats[256]; 
            swprintf_s(stats, L"CALLS: %llu   UPTIME: %llus   LAST: %lluus  AVG: %lluus  BEST: %lluus", 
                       StatsTracker::Instance().TotalCalls(), 
                       StatsTracker::Instance().GetUptimeSec(), 
                       StatsTracker::Instance().LastLatency(),
                       StatsTracker::Instance().AvgLatency(),
                       StatsTracker::Instance().BestLatency());
            TextOutW(mdc, 240, 55, stats, (int)wcslen(stats));

            // Telemetry Section
            SetTextColor(mdc, CLR_CHILI_RED);
            const wchar_t* telemetryText = L"[ REAL-TIME TELEMETRY ]";
            TextOutW(mdc, 20, 95, telemetryText, (int)wcslen(telemetryText));

            HBRUSH barBrush = CreateSolidBrush(CLR_NEON_GREEN);
            for (int i = 0; i < HIST_BUCKETS; ++i) {
                int barWidth = 30 + (int)(StatsTracker::Instance().GetHistCount(i) * 12);
                if (barWidth > 320) barWidth = 320;

                RECT barRect = { 130, 120 + (i * 22), 130 + barWidth, 120 + (i * 22) + 14 };
                FillRect(mdc, &barRect, barBrush);

                wchar_t bucketLabel[32];
                if (i < 3) {
                    swprintf_s(bucketLabel, L"<%dus : %ld", (i + 1) * 20, StatsTracker::Instance().GetHistCount(i));
                } else if (i == 3) {
                    swprintf_s(bucketLabel, L"<100us : %ld", StatsTracker::Instance().GetHistCount(i));
                } else {
                    swprintf_s(bucketLabel, L">=100us: %ld", StatsTracker::Instance().GetHistCount(i));
                }
                SetTextColor(mdc, CLR_TEXT_DIM);
                TextOutW(mdc, 20, 120 + (i * 22), bucketLabel, (int)wcslen(bucketLabel));
            }
            DeleteObject(barBrush);

            // System Logs Section
            SetTextColor(mdc, CLR_CHILI_RED);
            const wchar_t* logText = L"[ SYSTEM LOG ]";
            TextOutW(mdc, 20, 245, logText, (int)wcslen(logText));

            auto logs = MemoryLogger::Instance().GetRecentLogs();
            for (size_t i = 0; i < logs.size() && i < 11; ++i) {
                SetTextColor(mdc, CLR_NEON_GREEN);
                wchar_t line[300]; 
                swprintf_s(line, L"%ls [%ls] %ls", logs[i].timestamp.c_str(), logs[i].level.c_str(), logs[i].message.c_str());
                TextOutW(mdc, 20, 270 + (int)i * 20, line, (int)wcslen(line));
            }

            // Blit buffer to screen
            BitBlt(hdc, 0, 0, r.right, r.bottom, mdc, 0, 0, SRCCOPY); 

            // Clean up GDI objects cleanly to eliminate handle leaks
            SelectObject(mdc, oldPen);
            DeleteObject(linePen);
            SelectObject(mdc, oldFont);
            SelectObject(mdc, oldBmp); 
            DeleteObject(b); 
            DeleteDC(mdc);
        }

        HINSTANCE hInst_ = nullptr;
        HWND hwnd = nullptr; 
        HFONT font = nullptr; 
        bool visible = false;
        NOTIFYICONDATAW nid_ = {};
        HWINEVENTHOOK hook_ = nullptr;
        UINT wm_shellhook_ = 0;
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// APPLICATION ENTRY POINT
// ─────────────────────────────────────────────────────────────────────────────
#ifndef BENCHMARK
int WINAPI wWinMain(HINSTANCE h, HINSTANCE, LPWSTR, int) {
    // Single-instance enforcement
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, SL::APP_MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    timeBeginPeriod(1);

    SL::StatsTracker::Instance().Initialize();
    SL::Engine::Instance().Initialize();

    if (!SL::Terminal::Instance().Create(h)) {
        if (hMutex) CloseHandle(hMutex);
        timeEndPeriod(1);
        return 1;
    }

    HWND w = SL::Terminal::Instance().GetHwnd();
    RegisterHotKey(w, SL::HK_TOGGLE_DASHBOARD, MOD_NOREPEAT, VK_F9);
    RegisterHotKey(w, SL::HK_PAUSE_RESUME,     MOD_NOREPEAT, VK_F11);
    RegisterHotKey(w, SL::HK_EMERGENCY_EXIT,   MOD_NOREPEAT, VK_F12);

    LOG_INF(L"Spicy Lamar v4.0 Online. RingCentral Auto-Answer active.");

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }

    UnregisterHotKey(w, SL::HK_TOGGLE_DASHBOARD);
    UnregisterHotKey(w, SL::HK_PAUSE_RESUME);
    UnregisterHotKey(w, SL::HK_EMERGENCY_EXIT);

    timeEndPeriod(1);
    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    return (int)m.wParam;
}
#endif
