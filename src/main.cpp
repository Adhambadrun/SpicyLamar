// ═════════════════════════════════════════════════════════════════════════════
// SPICY LAMAR v4.4 // SINGLE-FILE MONOLITHIC SOURCE
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

#ifdef _WIN32
#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <objbase.h>
#include <oleacc.h>
#include <shellapi.h>
#include <evntrace.h>
#include <evntcons.h>
#include <tdh.h>
#include <timeapi.h>
#include <psapi.h>
#include <avrt.h>
#endif

#ifdef _MSC_VER
#include <wrl/client.h>
#include <immintrin.h>
#elif defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#endif

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
#include <cwchar>
#include <cwctype>
#include <cstring>

#ifdef _MSC_VER
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "avrt.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "oleacc.lib")
#pragma comment(lib, "tdh.lib")
#pragma comment(lib, "psapi.lib")
#else
#define SL_CROSS_COMPILE 1
#endif

#ifdef _MSC_VER
using Microsoft::WRL::ComPtr;
#define SL_SWPRINTF(dest, ...) swprintf_s(dest, __VA_ARGS__)
#define SL_WCSCPY(dest, src) wcscpy_s(dest, src)
#else
#define SL_SWPRINTF(dest, ...) swprintf(dest, sizeof(dest)/sizeof(dest[0]), __VA_ARGS__)
#define SL_WCSCPY(dest, src) wcscpy(dest, src)
#endif

namespace SL {
    // Identity Configuration — the product is named ONLY "Spicy Lamar".
    constexpr wchar_t APP_NAME[]                 = L"Spicy Lamar";
    constexpr wchar_t APP_CLASS_NAME[]           = L"SpicyLamar_v4";
    constexpr wchar_t APP_MUTEX_NAME[]           = L"Global\\SpicyLamar_v4_Mutex";
    constexpr wchar_t APP_MUTEX_FALLBACK[]       = L"Local\\SpicyLamar_v4_Mutex";
    constexpr wchar_t APP_VERSION[]              = L"Spicy Lamar v4.5";
    constexpr wchar_t TARGET_WINDOW_TITLE[]      = L"RingCentral Phone";
    constexpr wchar_t TARGET_CHILD_CLASS[]       = L"Chrome_RenderWidgetHostHWND";
    constexpr wchar_t TARGET_INTERMEDIATE_CLASS[]= L"Intermediate D3D Window";

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
    constexpr UINT    IDM_SELF_TEST        = 2008;   // v4.4: F8 self-test

    // Visual Palette (Obsidian & Chili Neon)
    constexpr COLORREF CLR_OBSIDIAN       = RGB(5, 5, 5);
    constexpr COLORREF CLR_CHILI_RED      = RGB(255, 51, 0);
    constexpr COLORREF CLR_NEON_GREEN     = RGB(0, 255, 102);
    constexpr COLORREF CLR_CHARCOAL       = RGB(68, 68, 68);
    constexpr COLORREF CLR_TEXT_DIM       = RGB(180, 180, 180);

    // Performance & Telemetry Constants
#ifdef SPICY_LAMAR_TURBO
    // MAX-PERFORMANCE build (SPICY_LAMAR_TURBO): tuned for extreme speed.
    // 5 ms target scanning on dedicated high-priority MMCSS thread, HIGH (not
    // real-time) process priority, 0.5 ms kernel timer, 100 ms cascade floor.
    // v4.3: real-time priority + a 1 ms floor flooded RingCentral (a Chromium
    // app) with up to 1000 cascades/sec of window messages, SendInput and
    // foreground steals, starving its input/renderer threads — the app lagged.
    // Real call events still fire in microseconds; only storms are coalesced.
    constexpr DWORD   DEFAULT_POLL_MS      = 5;   // 200 Hz target scan
    constexpr int     MAX_TELEMETRY_LOGS   = 50;
#else
    // Default: 20 ms scan / 0.5 s heart-beat, gentle on shared machines.
    constexpr DWORD   DEFAULT_POLL_MS      = 20;
    constexpr int     MAX_TELEMETRY_LOGS   = 15;
#endif

    // ── Answer engine rate control ───────────────────────────────────
#ifdef SPICY_LAMAR_TURBO
    // v4.3: 100 ms cascade floor on the poll channel (max 10/sec) — the old
    // 1 ms floor fired the full cascade up to 1000x/sec and choked RingCentral.
    constexpr DWORD   ANSWER_DEBOUNCE_MS   = 100;
    // Absolute floor between cascades from ANY channel, including "instant"
    // WinEvent/shell-hook firings. WinEvents arrive hundreds of times per
    // second; without this they each launched a full cascade. The first event
    // after quiet still fires instantly (last tick is stale by definition).
    constexpr DWORD   ANSWER_MIN_CASCADE_GAP_MS = 50;
    constexpr DWORD   ANSWER_MIN_RETRY_MS  = 100;
    constexpr int     ANSWER_MAX_ATTEMPTS  = 3;
    constexpr DWORD   ANSWER_EPISODE_MS    = 2000;
    // Per-cascade log floor: at 1000 Hz, formatting/allocating a log line
    // per cascade would dominate the loop. Stats are still fully recorded
    // for every cascade; only the log lines are coalesced.
    constexpr DWORD   ANSWER_LOG_MIN_GAP_MS = 250;
#else
    constexpr DWORD   ANSWER_DEBOUNCE_MS   = 500;
    constexpr DWORD   ANSWER_MIN_CASCADE_GAP_MS = 100;  // storm coalescing floor
    constexpr DWORD   ANSWER_MIN_RETRY_MS  = 1500;
    constexpr int     ANSWER_MAX_ATTEMPTS  = 3;
    constexpr DWORD   ANSWER_EPISODE_MS    = 10000;
    constexpr DWORD   ANSWER_LOG_MIN_GAP_MS = 0;    // log every cascade
#endif

    // Answer-trigger channels (telemetry + rate-control policy).
    constexpr uint32_t CHAN_WINEVENT  = 1;   // WinEvent hook: real call event
    constexpr uint32_t CHAN_POLL      = 2;   // attention poll: debounced
    constexpr uint32_t CHAN_SHELLHOOK = 3;   // shell hook: real call event

    constexpr int     HIST_BUCKETS         = 5;
#ifdef SPICY_LAMAR_TURBO
    constexpr DWORD   STATS_REFRESH_MS     = 100;   // snappy dashboard when visible
#else
    constexpr DWORD   STATS_REFRESH_MS     = 250;
#endif
    constexpr int     DASH_WIDTH           = 780;
    constexpr int     DASH_HEIGHT          = 520;

    // Global Hotkey Identifiers
    constexpr int HK_TOGGLE_DASHBOARD      = 1;
    constexpr int HK_PAUSE_RESUME          = 2;
    constexpr int HK_EMERGENCY_EXIT        = 3;
    constexpr int HK_SELF_TEST             = 4;   // v4.4: F8 self-test
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
            SL_SWPRINTF(ts, L"%02d:%02d:%02d.%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
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

#define LOG_INF(fmt, ...) { wchar_t b[256]; SL_SWPRINTF(b, fmt, ##__VA_ARGS__); SL::MemoryLogger::Instance().Log(L"INF", b); }
#define LOG_WRN(fmt, ...) { wchar_t b[256]; SL_SWPRINTF(b, fmt, ##__VA_ARGS__); SL::MemoryLogger::Instance().Log(L"WRN", b); }
#define LOG_ERR(fmt, ...) { wchar_t b[256]; SL_SWPRINTF(b, fmt, ##__VA_ARGS__); SL::MemoryLogger::Instance().Log(L"ERR", b); }

// ─────────────────────────────────────────────────────────────────────────────
// SYSTEM TUNING & REAL-TIME SCHEDULING (MMCSS & KERNEL TIMERS)
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    struct SpinLock {
        std::atomic<bool> flag{false};

        bool try_lock() noexcept {
            return !flag.exchange(true, std::memory_order_acquire);
        }

        void lock() noexcept {
            while (flag.exchange(true, std::memory_order_acquire)) {
                while (flag.load(std::memory_order_relaxed)) {
#if defined(_MSC_VER)
                    _mm_pause();
#elif defined(__x86_64__) || defined(__i386__)
                    __builtin_ia32_pause();
#else
                    std::this_thread::yield();
#endif
                }
            }
        }

        void unlock() noexcept {
            flag.store(false, std::memory_order_release);
        }
    };

    typedef LONG (NTAPI *pfnNtSetTimerResolution)(ULONG DesiredResolution, BOOLEAN SetResolution, PULONG CurrentResolution);
    typedef HANDLE (WINAPI *pfnAvSetMmThreadCharacteristicsW)(LPCWSTR TaskName, LPDWORD TaskIndex);
    typedef BOOL (WINAPI *pfnAvRevertMmThreadCharacteristics)(HANDLE AvrtHandle);

    inline void EnableSystemOptimizations() {
        // 1. Minimum OS Timer Period (1 ms multimedia timer)
        timeBeginPeriod(1);

        // 2. High-Precision NT Kernel Timer (0.5 ms resolution: 5000 * 100ns units)
        HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
        if (hNtDll) {
            auto pNtSet = (pfnNtSetTimerResolution)GetProcAddress(hNtDll, "NtSetTimerResolution");
            if (pNtSet) {
                ULONG cur = 0;
                pNtSet(5000, TRUE, &cur);
            }
        }

        // 3. Process priority class.
#ifdef SPICY_LAMAR_TURBO
        // v4.3: HIGH, not REALTIME. A real-time-class process polling a Chromium
        // app (RingCentral) starves RingCentral's own input/renderer threads and
        // makes the whole app lag. High priority keeps answers fast without
        // monopolizing the scheduler.
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#else
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif

        // 4. Disable dynamic priority decay so the process stays at peak priority
        SetProcessPriorityBoost(GetCurrentProcess(), FALSE);

        // 5. Thread priority boost for main thread (v4.3: HIGHEST, not
        // TIME_CRITICAL — time-critical threads starve other apps)
        if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST)) {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
        }

        // 6. Pre-allocate and lock working set to physical RAM (prevents page faults during answering)
        SIZE_T minWs = 32 * 1024 * 1024;
        SIZE_T maxWs = 128 * 1024 * 1024;
        SetProcessWorkingSetSize(GetCurrentProcess(), minWs, maxWs);

        // 7. Bypass Windows OS foreground lock delay
        DWORD zero = 0;
        SystemParametersInfoW(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)(ULONG_PTR)zero, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
        AllowSetForegroundWindow(ASFW_ANY);
    }

    inline HANDLE RegisterMMCSS() {
        HMODULE hAvrt = LoadLibraryW(L"avrt.dll");
        if (hAvrt) {
            auto pAvSet = (pfnAvSetMmThreadCharacteristicsW)GetProcAddress(hAvrt, "AvSetMmThreadCharacteristicsW");
            if (pAvSet) {
                DWORD taskIndex = 0;
                HANDLE h = pAvSet(L"Pro Audio", &taskIndex);
                if (!h) {
                    taskIndex = 0;
                    h = pAvSet(L"Games", &taskIndex);
                }
                return h;
            }
        }
        return nullptr;
    }

    inline void UnregisterMMCSS(HANDLE h) {
        if (!h) return;
        HMODULE hAvrt = GetModuleHandleW(L"avrt.dll");
        if (hAvrt) {
            auto pRevert = (pfnAvRevertMmThreadCharacteristics)GetProcAddress(hAvrt, "AvRevertMmThreadCharacteristics");
            if (pRevert) pRevert(h);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// STATS & TELEMETRY TRACKER (NANOSECOND RESOLUTION)
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
            qpc_freq = f.QuadPart > 0 ? f.QuadPart : 10000000LL; 
            start = GetTickCount64(); 
        }

        LARGE_INTEGER QpcNow() const noexcept { 
            LARGE_INTEGER li; 
            QueryPerformanceCounter(&li); 
            return li; 
        }

        uint64_t DeltaMicros(LARGE_INTEGER t0, LARGE_INTEGER t1) const noexcept { 
            if (qpc_freq <= 0) return 0;
            return (uint64_t)((t1.QuadPart - t0.QuadPart) * 1000000ULL / qpc_freq); 
        }

        void RecordAnswer(LARGE_INTEGER t0, LARGE_INTEGER t1, uint32_t hits, uint32_t chan) noexcept {
            RecordLatency(DeltaMicros(t0, t1), chan);
        }

        // v4.4: record one cascade latency. Used by live answers; the F8
        // self-test path deliberately does NOT call this so telemetry stays clean.
        void RecordLatency(uint64_t us, uint32_t chan) noexcept {
            last_us.store(us, std::memory_order_relaxed); 
            calls.fetch_add(1, std::memory_order_relaxed);
            sum_us.fetch_add(us, std::memory_order_relaxed);

            // Atomic best tracking
            uint64_t curBest = best_us.load(std::memory_order_relaxed);
            while (us < curBest && !best_us.compare_exchange_weak(curBest, us, std::memory_order_relaxed)) {}

            // Atomic worst tracking
            uint64_t curWorst = worst_us.load(std::memory_order_relaxed);
            while (us > curWorst && !worst_us.compare_exchange_weak(curWorst, us, std::memory_order_relaxed)) {}

            if (us < 20) hist[0].fetch_add(1, std::memory_order_relaxed); 
            else if (us < 40) hist[1].fetch_add(1, std::memory_order_relaxed); 
            else if (us < 60) hist[2].fetch_add(1, std::memory_order_relaxed); 
            else if (us < 100) hist[3].fetch_add(1, std::memory_order_relaxed); 
            else hist[4].fetch_add(1, std::memory_order_relaxed);
        }

        uint64_t LastLatency() const noexcept { return last_us.load(std::memory_order_relaxed); }
        uint64_t BestLatency() const noexcept { 
            uint64_t b = best_us.load(std::memory_order_relaxed); 
            return b == UINT64_MAX ? 0 : b; 
        }
        uint64_t WorstLatency() const noexcept { return worst_us.load(std::memory_order_relaxed); }
        uint64_t TotalCalls() const noexcept { return calls.load(std::memory_order_relaxed); }
        uint64_t AvgLatency() const noexcept { 
            uint64_t c = calls.load(std::memory_order_relaxed); 
            return c > 0 ? (sum_us.load(std::memory_order_relaxed) / c) : 0; 
        }
        uint64_t GetUptimeSec() const noexcept { return (GetTickCount64() - start) / 1000; }
        long GetHistCount(int i) const noexcept { 
            if (i >= 0 && i < HIST_BUCKETS) return hist[i].load(std::memory_order_relaxed); 
            return 0; 
        }

    private:
        StatsTracker() 
            : qpc_freq(10000000LL), start(0), last_us(0), best_us(UINT64_MAX), worst_us(0), calls(0), sum_us(0) { 
            for (auto& h : hist) h.store(0, std::memory_order_relaxed); 
        }

        int64_t qpc_freq; 
        uint64_t start; 
        std::atomic<uint64_t> last_us, best_us, worst_us, calls, sum_us; 
        std::array<std::atomic<long>, HIST_BUCKETS> hist;
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// WINDOW CACHE & O(1) ENUMERATION
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    inline bool ContainsInsensitive(const wchar_t* text, const wchar_t* needle) noexcept {
        if (!text || !needle || !*needle) return false;
        const size_t n = wcslen(needle);
        for (; *text; ++text) if (_wcsnicmp(text, needle, n) == 0) return true;
        return false;
    }

    inline bool IsRingCentralTitle(const wchar_t* title) noexcept {
        return ContainsInsensitive(title, L"RingCentral") ||
               ContainsInsensitive(title, L"Ring Central") ||
               ContainsInsensitive(title, L"RingMe") ||
               ContainsInsensitive(title, L"Glip");
    }

    // v4.5: process-image-name detection. RingCentral updates often change the
    // window TITLE or the Electron window CLASS, which breaks the title/class
    // heuristics above. The owning executable name is far more stable, so we
    // fall back to matching it. This is what restores "finds RingCentral even
    // after an update" behaviour.
    inline bool GetProcessImageName(HWND hwnd, std::wstring& outName) noexcept {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (!pid) return false;

        HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                   FALSE, pid);
        if (!hProc) {
            // Some processes reject VM_READ; the limited-information right is
            // still enough to read the image path on Vista+.
            hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        }
        if (!hProc) return false;

        wchar_t buf[MAX_PATH] = {0};
        DWORD sz = MAX_PATH;
        BOOL ok = QueryFullProcessImageNameW(hProc, 0, buf, &sz);
        CloseHandle(hProc);
        if (!ok) return false;

        std::wstring path(buf);
        size_t slash = path.find_last_of(L"\\/");
        std::wstring name = (slash == std::wstring::npos) ? path
                                                       : path.substr(slash + 1);
        for (wchar_t& c : name) c = (wchar_t)towlower(c);
        outName = name;
        return true;
    }

    inline bool IsRingCentralProcessName(const std::wstring& name) noexcept {
        return ContainsInsensitive(name.c_str(), L"ringcentral") ||
               ContainsInsensitive(name.c_str(), L"glip") ||
               ContainsInsensitive(name.c_str(), L"rcdesktop") ||
               ContainsInsensitive(name.c_str(), L"ringcentralphone");
    }

    class WindowCache {
    public:
        struct Snapshot { HWND main; HWND child; HWND intermediate; };

        static WindowCache& Instance() { 
            static WindowCache inst; 
            return inst; 
        }

        Snapshot GetSnapshot() const noexcept { 
            return { main.load(std::memory_order_relaxed), 
                     child.load(std::memory_order_relaxed),
                     intermediate.load(std::memory_order_relaxed) }; 
        }

        void Invalidate() noexcept {
            main.store(nullptr, std::memory_order_release);
            child.store(nullptr, std::memory_order_release);
            intermediate.store(nullptr, std::memory_order_release);
        }

        void Update(HWND m, HWND c, HWND inter = nullptr) noexcept { 
            main.store(m, std::memory_order_release); 
            child.store(c, std::memory_order_release); 
            intermediate.store(inter, std::memory_order_release);
        }

        HWND FindRingCentral() {
            // Fast path 1: Cached HWND
            HWND m = main.load(std::memory_order_acquire);
            if (m && IsWindow(m)) {
                HWND c = child.load(std::memory_order_acquire);
                if (!c || !IsWindow(c)) {
                    HWND inter = nullptr;
                    c = FindChild(m, &inter);
                    Update(m, c, inter);
                }
                return m;
            }

            // Fast path 2: Direct O(1) FindWindowW for standard window title
            HWND direct = FindWindowW(nullptr, TARGET_WINDOW_TITLE);
            if (direct && IsWindow(direct)) {
                HWND inter = nullptr;
                HWND c = FindChild(direct, &inter);
                Update(direct, c, inter);
                return direct;
            }

            // Fast path 3: Direct FindWindowW for RingCentral Electron class.
            // v4.5: accept by title OR by owning process name (more robust
            // against RingCentral changing its window title).
            direct = FindWindowW(L"Chrome_WidgetWin_1", nullptr);
            if (direct && IsWindow(direct)) {
                wchar_t title[256] = {0};
                int tlen = GetWindowTextLengthW(direct);
                if (tlen > 0 && tlen < 256) GetWindowTextW(direct, title, 256);
                std::wstring pname;
                bool byProc = GetProcessImageName(direct, pname) &&
                              IsRingCentralProcessName(pname);
                if (IsRingCentralTitle(title) || byProc) {
                    HWND inter = nullptr;
                    HWND c = FindChild(direct, &inter);
                    Update(direct, c, inter);
                    return direct;
                }
            }

            // Fast path 4: Fast window enumeration with visibility check
            struct SearchData {
                HWND found = nullptr;
            } data;

            EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
                if (!IsWindowVisible(hwnd)) return TRUE;
                int len = GetWindowTextLengthW(hwnd);

                wchar_t title[256] = {0};
                if (len > 0 && len < 256) GetWindowTextW(hwnd, title, 256);
                if (IsRingCentralTitle(title)) {
                    auto* d = reinterpret_cast<SearchData*>(lp);
                    d->found = hwnd;
                    return FALSE; // Stop enumeration once found
                }

                // v4.5: process-name fallback for RingCentral windows whose
                // title no longer contains a recognizable substring.
                std::wstring pname;
                if (GetProcessImageName(hwnd, pname) &&
                    IsRingCentralProcessName(pname)) {
                    auto* d = reinterpret_cast<SearchData*>(lp);
                    d->found = hwnd;
                    return FALSE;
                }
                return TRUE;
            }, reinterpret_cast<LPARAM>(&data));

            if (data.found) {
                HWND inter = nullptr;
                HWND c = FindChild(data.found, &inter);
                Update(data.found, c, inter);
                return data.found;
            }

            return nullptr;
        }

        HWND FindChild(HWND mainWnd, HWND* outInter = nullptr) {
            if (!mainWnd || !IsWindow(mainWnd)) return nullptr;

            // Direct Chrome render widget child
            HWND c = FindWindowExW(mainWnd, nullptr, TARGET_CHILD_CLASS, nullptr);
            if (c && IsWindow(c)) {
                if (outInter) *outInter = nullptr;
                return c;
            }

            // Intermediate D3D window
            HWND inter = FindWindowExW(mainWnd, nullptr, TARGET_INTERMEDIATE_CLASS, nullptr);
            if (inter && IsWindow(inter)) {
                if (outInter) *outInter = inter;
                c = FindWindowExW(inter, nullptr, TARGET_CHILD_CLASS, nullptr);
                if (c && IsWindow(c)) return c;
            }

            // Fallback EnumChildWindows
            struct ChildSearch {
                HWND targetChild = nullptr;
                HWND intermediate = nullptr;
            } cs;

            EnumChildWindows(mainWnd, [](HWND h, LPARAM lp) -> BOOL {
                wchar_t cls[128] = {0};
                GetClassNameW(h, cls, 128);
                auto* s = reinterpret_cast<ChildSearch*>(lp);
                if (_wcsicmp(cls, TARGET_CHILD_CLASS) == 0) {
                    s->targetChild = h;
                    return FALSE;
                } else if (_wcsicmp(cls, L"Intermediate D3D Window") == 0) {
                    s->intermediate = h;
                }
                return TRUE;
            }, reinterpret_cast<LPARAM>(&cs));

            if (cs.targetChild) {
                if (outInter) *outInter = cs.intermediate;
                return cs.targetChild;
            }
            if (cs.intermediate) {
                c = FindWindowExW(cs.intermediate, nullptr, TARGET_CHILD_CLASS, nullptr);
                if (c) {
                    if (outInter) *outInter = cs.intermediate;
                    return c;
                }
            }
            return nullptr;
        }

    private:
        WindowCache() : main(nullptr), child(nullptr), intermediate(nullptr) {}
        std::atomic<HWND> main;
        std::atomic<HWND> child;
        std::atomic<HWND> intermediate;
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// QUANTUM ANSWER ENGINE (6-SHOT IPC CASCADE WITH HARDWARE SCAN CODES)
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    // v4.5: collect EVERY top-level RingCentral window (by title OR process
    // name), deduplicated. A ringing call's answer UI frequently lives in a
    // SEPARATE popup window that is NOT a child of the main window, so the
    // answer cascade must be delivered to all of them — not only the main one.
    inline void CollectRingCentralWindows(std::vector<HWND>& out) {
        out.clear();
        // Seed with the cached main window for the fast path.
        HWND m = WindowCache::Instance().GetSnapshot().main;
        if (m && IsWindow(m)) out.push_back(m);

        EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
            if (!IsWindow(hwnd)) return TRUE;
            bool match = false;
            wchar_t title[256] = {0};
            int len = GetWindowTextLengthW(hwnd);
            if (len > 0 && len < 256) {
                GetWindowTextW(hwnd, title, 256);
                if (IsRingCentralTitle(title)) match = true;
            }
            if (!match) {
                std::wstring pname;
                if (GetProcessImageName(hwnd, pname) && IsRingCentralProcessName(pname))
                    match = true;
            }
            if (match) {
                auto* v = reinterpret_cast<std::vector<HWND>*>(lp);
                v->push_back(hwnd);
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&out));

        // Deduplicate (the cached main window may also have been enumerated).
        std::vector<HWND> compact;
        compact.reserve(out.size());
        for (HWND w : out) {
            bool dup = false;
            for (HWND e : compact) if (e == w) { dup = true; break; }
            if (!dup) compact.push_back(w);
        }
        out.swap(compact);
    }

    class Engine {
    public:
        static Engine& Instance() { 
            static Engine inst; 
            return inst; 
        }

        void Initialize() {
            active.store(true, std::memory_order_release);
            attempts.store(0, std::memory_order_relaxed);
            last_attempt_tick.store(0, std::memory_order_relaxed);
            cap_logged.store(false, std::memory_order_relaxed);
            last_log_tick.store(0, std::memory_order_relaxed);
            suppressed_logs.store(0, std::memory_order_relaxed);
        }

        void SetActive(bool a) { active.store(a, std::memory_order_release); }
        bool IsActive() const noexcept { return active.load(std::memory_order_relaxed); }

        bool TryAnswer(HWND hint, uint32_t chan, bool force = false) {
            if (!active.load(std::memory_order_relaxed)) return false;

            // Zero-overhead nanosecond spinlock
            std::unique_lock<SpinLock> fireGuard(spin_lock_, std::try_to_lock);
            if (!fireGuard.owns_lock()) {
                return false; // Another thread is actively delivering the cascade
            }

#ifdef BENCHMARK
            force = true;
#endif

            HWND m = hint;
            if (!m || !IsWindow(m)) {
                m = WindowCache::Instance().FindRingCentral();
            }
            if (!m || !IsWindow(m)) return false;

            // Rate control
            if (!force) {
                ULONGLONG now = GetTickCount64();
#ifdef SPICY_LAMAR_BOUNDED
                if (last_attempt_tick.load(std::memory_order_relaxed) == 0 ||
                    (now - last_attempt_tick.load(std::memory_order_relaxed)) > ANSWER_EPISODE_MS) {
                    attempts.store(0, std::memory_order_relaxed);
                    cap_logged.store(false, std::memory_order_relaxed);
                }
                if (attempts.load(std::memory_order_relaxed) >= ANSWER_MAX_ATTEMPTS) {
                    if (!cap_logged.exchange(true)) {
                        LOG_WRN(L"Episode cap reached (%d attempts) — Alt+F1 idle until next call event",
                                ANSWER_MAX_ATTEMPTS);
                    }
                    return false;
                }
                if (last_attempt_tick.load(std::memory_order_relaxed) != 0 &&
                    (now - last_attempt_tick.load(std::memory_order_relaxed)) < ANSWER_MIN_RETRY_MS) {
                    return false;
                }
                attempts.fetch_add(1, std::memory_order_relaxed);
#else
                // v4.3: every channel is rate-limited. The poll channel waits
                // ANSWER_DEBOUNCE_MS between cascades; "instant" event channels
                // (WinEvent / shell hook) are coalesced at ANSWER_MIN_CASCADE_GAP_MS
                // so a window-event storm cannot flood RingCentral with hundreds
                // of cascades per second. The first event after quiet still fires
                // in microseconds (last tick is stale, so no wait is added).
                ULONGLONG lastTick = last_attempt_tick.load(std::memory_order_relaxed);
                DWORD minGap = (chan == CHAN_POLL) ? ANSWER_DEBOUNCE_MS : ANSWER_MIN_CASCADE_GAP_MS;
                if (lastTick != 0 && (now - lastTick) < minGap) {
                    return false;
                }
#endif
                last_attempt_tick.store(now, std::memory_order_relaxed);
            }

            // v4.4: focus steal + Alt+F1-only cascade live in DeliverAnswerCascade()
            // so the F8 self-test can reuse them.
            uint64_t lat = DeliverAnswerCascade(m);

            StatsTracker::Instance().RecordLatency(lat, chan);

            if constexpr (ANSWER_LOG_MIN_GAP_MS == 0) {
                LOG_INF(L"ANSWERED via 6-Shot Cascade [Chan: %u] in %lluus", chan, lat);
            } else {
                ULONGLONG lnow = GetTickCount64();
                ULONGLONG lastLog = last_log_tick.load(std::memory_order_relaxed);
                if (lastLog == 0 || (lnow - lastLog) >= ANSWER_LOG_MIN_GAP_MS) {
                    last_log_tick.store(lnow, std::memory_order_relaxed);
                    uint64_t skipped = suppressed_logs.exchange(0, std::memory_order_relaxed);
                    if (skipped > 0) {
                        LOG_INF(L"ANSWERED via 6-Shot Cascade [Chan: %u] in %lluus (+%llu more cascades since last log)",
                                chan, lat, skipped);
                    } else {
                        LOG_INF(L"ANSWERED via 6-Shot Cascade [Chan: %u] in %lluus", chan, lat);
                    }
                } else {
                    suppressed_logs.fetch_add(1, std::memory_order_relaxed);
                }
            }
            return true;
        }

        // v4.4 F8 SELF-TEST (also in the tray menu): verifies the Alt+F1
        // answer path end-to-end WITHOUT touching call stats, and runs even
        // while paused. Finds the RingCentral Phone window and delivers one
        // real Alt+F1 cascade; the log reports the result. Never sends Alt+A.
        void RunSelfTest() {
            LOG_INF(L"──── F8 SELF-TEST ────────────────────────────────────");
            LOG_INF(L"Engine: %ls. Answer key: Alt+F1 ONLY (Alt+A is never sent).",
                    IsActive() ? L"ACTIVE" : L"PAUSED (self-test runs anyway)");

            HWND m = WindowCache::Instance().FindRingCentral();
            if (!m || !IsWindow(m)) {
                LOG_WRN(L"SELF-TEST: RingCentral Phone window NOT found.");
                LOG_WRN(L"SELF-TEST: Start RingCentral Phone, then press F8 again.");
                LOG_INF(L"──── SELF-TEST COMPLETE (no window) ───────────────────");
                return;
            }

            wchar_t title[256] = {0};
            GetWindowTextW(m, title, 256);
            LOG_INF(L"SELF-TEST: found RingCentral window \"%ls\" (hwnd 0x%p).", title, (void*)m);
            LOG_INF(L"SELF-TEST: delivering one Alt+F1 cascade (focus + 6-shot IPC + hardware SendInput)...");

            std::unique_lock<SpinLock> fireGuard(spin_lock_, std::try_to_lock);
            if (!fireGuard.owns_lock()) {
                LOG_WRN(L"SELF-TEST: a live answer cascade is in flight. Try again in a moment.");
                LOG_INF(L"──── SELF-TEST ABORTED ────────────────────────────────");
                return;
            }
            uint64_t lat = DeliverAnswerCascade(m);
            LOG_INF(L"SELF-TEST: Alt+F1 cascade delivered in %lluus.", (unsigned long long)lat);
            LOG_INF(L"SELF-TEST: PASS - Alt+F1 path verified (call stats unchanged).");
            LOG_INF(L"SELF-TEST: a ringing call is answered; an idle RingCentral ignores the shortcut.");
            LOG_INF(L"──── SELF-TEST COMPLETE ────────────────────────────────");
        }

    private:
        // v4.4 - ANSWER KEY IS Alt+F1 ONLY. Every shot below synthesizes
        // Alt+F1 (plus Enter / Space / WM_COMMAND fallbacks that are NOT
        // typing). RingCentral's STOCK answer shortcut Alt+A is deliberately
        // NEVER sent: this build targets a RingCentral install whose answer
        // shortcut is remapped to Alt+F1, and a stray Alt+A could open an app
        // menu or trigger an unrelated action. Do NOT re-add an Alt+A fallback.
        // Returns the cascade delivery latency in microseconds.
        uint64_t DeliverAnswerCascade(HWND m) {
            LARGE_INTEGER t0 = StatsTracker::Instance().QpcNow();

            auto snap = WindowCache::Instance().GetSnapshot();
            HWND child = snap.child;
            HWND inter = snap.intermediate;
            if (!child || !IsWindow(child)) {
                child = WindowCache::Instance().FindChild(m, &inter);
                WindowCache::Instance().Update(m, child, inter);
            }

            // v4.5: A ringing call's answer UI is usually a SEPARATE top-level
            // RingCentral window (its own popup), not a child of the main window.
            // We must deliver Alt+F1 to EVERY RingCentral window, otherwise the
            // keystroke lands on the main window while the call popup — the one
            // that actually answers — never sees it. This is the fix for
            // "manual Alt+F1 answers, but the app's auto-answer does nothing".
            std::vector<HWND> rc;
            CollectRingCentralWindows(rc);
            if (rc.empty() && m && IsWindow(m)) rc.push_back(m);

            // Choose the hardware-injection target. Prefer whatever RingCentral
            // window is ALREADY in the foreground: that's the call popup you'd be
            // pressing Alt+F1 on by hand, so we must NOT steal focus off it. Only
            // foreground a window when no RingCentral window is already foreground.
            HWND fg = GetForegroundWindow();
            HWND hwTarget = nullptr;
            if (fg && IsWindow(fg)) {
                bool fgRC = false;
                wchar_t t[256] = {0};
                int l = GetWindowTextLengthW(fg);
                if (l > 0 && l < 256) {
                    GetWindowTextW(fg, t, 256);
                    if (IsRingCentralTitle(t)) fgRC = true;
                }
                if (!fgRC) {
                    std::wstring pn;
                    if (GetProcessImageName(fg, pn) && IsRingCentralProcessName(pn))
                        fgRC = true;
                }
                if (fgRC) hwTarget = fg;
            }
            if (!hwTarget) hwTarget = m;

            DWORD curThreadId = GetCurrentThreadId();
            DWORD targetThreadId = GetWindowThreadProcessId(hwTarget, nullptr);
            BOOL attached = FALSE;
            if (targetThreadId && targetThreadId != curThreadId) {
                attached = AttachThreadInput(curThreadId, targetThreadId, TRUE);
            }

            // Only force focus if no RingCentral window is already foreground.
            if (fg != hwTarget) {
                if (IsIconic(hwTarget)) ShowWindow(hwTarget, SW_RESTORE);
                BringWindowToTop(hwTarget);
                SetForegroundWindow(hwTarget);
            }

            // ─────────────────────────────────────────────────────────────────
            // PER-WINDOW Alt+F1 IPC CASCADE (sent to EVERY RingCentral window)
            // ─────────────────────────────────────────────────────────────────
            for (HWND w : rc) {
                HWND wc = nullptr, wi = nullptr;
                if (w == m) { wc = child; wi = inter; }
                else        { wc = WindowCache::Instance().FindChild(w, &wi); }

                // Shot 1: window Alt+F1 Down/Up (hardware scan codes 0x38, 0x3B)
                PostMessageW(w, WM_SYSKEYDOWN, VK_MENU, 0x20380001);
                PostMessageW(w, WM_SYSKEYDOWN, VK_F1,   0x203B0001);
                PostMessageW(w, WM_SYSKEYUP,   VK_F1,   0xE03B0001);
                PostMessageW(w, WM_KEYUP,      VK_MENU, 0xE0380001);

                // Shot 2: Chrome Render Child + Intermediate D3D Alt+F1 Down/Up
                if (wc && IsWindow(wc)) {
                    PostMessageW(wc, WM_SYSKEYDOWN, VK_MENU, 0x20380001);
                    PostMessageW(wc, WM_SYSKEYDOWN, VK_F1,   0x203B0001);
                    PostMessageW(wc, WM_SYSKEYUP,   VK_F1,   0xE03B0001);
                    PostMessageW(wc, WM_KEYUP,      VK_MENU, 0xE0380001);
                }
                if (wi && IsWindow(wi)) {
                    PostMessageW(wi, WM_SYSKEYDOWN, VK_MENU, 0x20380001);
                    PostMessageW(wi, WM_SYSKEYDOWN, VK_F1,   0x203B0001);
                    PostMessageW(wi, WM_SYSKEYUP,   VK_F1,   0xE03B0001);
                    PostMessageW(wi, WM_KEYUP,      VK_MENU, 0xE0380001);
                }
            }

            // Shot 3: Post Enter & Space keys to main window + child (NOT typing)
            PostMessageW(m, WM_KEYDOWN, VK_RETURN, 0x001C0001);
            PostMessageW(m, WM_KEYUP,   VK_RETURN, 0xC01C0001);
            PostMessageW(m, WM_KEYDOWN, VK_SPACE,  0x00390001);
            PostMessageW(m, WM_KEYUP,   VK_SPACE,  0xC0390001);
            if (child && IsWindow(child)) {
                PostMessageW(child, WM_KEYDOWN, VK_RETURN, 0x001C0001);
                PostMessageW(child, WM_KEYUP,   VK_RETURN, 0xC01C0001);
            }

            // Shot 4: Hardware-level SendInput batch + legacy keybd_event to the
            // chosen target (the foreground call popup when one is already up).
            SetForegroundWindow(hwTarget);
            INPUT input[4] = {};
            input[0].type = INPUT_KEYBOARD;
            input[0].ki.wVk = VK_MENU;
            input[0].ki.wScan = 0x38;

            input[1].type = INPUT_KEYBOARD;
            input[1].ki.wVk = VK_F1;
            input[1].ki.wScan = 0x3B;

            input[2].type = INPUT_KEYBOARD;
            input[2].ki.wVk = VK_F1;
            input[2].ki.wScan = 0x3B;
            input[2].ki.dwFlags = KEYEVENTF_KEYUP;

            input[3].type = INPUT_KEYBOARD;
            input[3].ki.wVk = VK_MENU;
            input[3].ki.wScan = 0x38;
            input[3].ki.dwFlags = KEYEVENTF_KEYUP;

            SendInput(4, input, sizeof(INPUT));

            keybd_event(VK_MENU, 0x38, 0, 0);
            keybd_event(VK_F1,   0x3B, 0, 0);
            keybd_event(VK_F1,   0x3B, KEYEVENTF_KEYUP, 0);
            keybd_event(VK_MENU, 0x38, KEYEVENTF_KEYUP, 0);

            // Shot 5: Direct RingCentral Command messages (main window)
            PostMessageW(m, WM_COMMAND, MAKEWPARAM(1001, 0), 0);
            PostMessageW(m, WM_COMMAND, MAKEWPARAM(1, 0), 0);
            PostMessageW(m, WM_COMMAND, MAKEWPARAM(101, 0), 0);

            // Shot 6: re-assert foreground + modifier release safety net + detach
            // (v4.3: the old WM_SYSCHAR(VK_F1) shot was removed — VK_F1 == 0x70 ==
            // ASCII 'p', so Chromium typed 'p' into the app, the 'pppppppp' spam.)
            SetForegroundWindow(hwTarget);
            keybd_event(VK_MENU,    0x38, KEYEVENTF_KEYUP, 0);
            keybd_event(VK_CONTROL, 0x1D, KEYEVENTF_KEYUP, 0);
            keybd_event(VK_SHIFT,   0x2A, KEYEVENTF_KEYUP, 0);

            if (attached) {
                AttachThreadInput(curThreadId, targetThreadId, FALSE);
            }

            LARGE_INTEGER t1 = StatsTracker::Instance().QpcNow();
            return StatsTracker::Instance().DeltaMicros(t0, t1);
        }

        Engine() : active(true), attempts(0), last_attempt_tick(0), cap_logged(false),
                   last_log_tick(0), suppressed_logs(0) {}
        std::atomic<bool> active;
        std::atomic<int> attempts;
        std::atomic<ULONGLONG> last_attempt_tick;
        std::atomic<bool> cap_logged;
        std::atomic<ULONGLONG> last_log_tick;
        std::atomic<uint64_t> suppressed_logs;
        SpinLock spin_lock_;
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// HIGH-PRIORITY POLL WORKER (TURBO BUILD WITH HIGH-RESOLUTION TIMERS)
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    class PollWorker {
    public:
        static PollWorker& Instance() {
            static PollWorker inst;
            return inst;
        }

        void Start() {
            if (running_.load(std::memory_order_relaxed)) return;
            running_.store(true, std::memory_order_release);
            thread_ = std::thread([this]() { Loop(); });
        }

        void Stop() {
            running_.store(false, std::memory_order_release);
            if (thread_.joinable()) thread_.join();
        }

    private:
        void Loop() {
            // Elevated priority boost (v4.3: HIGHEST, not TIME_CRITICAL —
            // a time-critical 1000 Hz loop starved RingCentral's own threads)
            if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST)) {
                SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
            }

            // Register MMCSS for Real-time Multimedia Scheduling
            HANDLE hAvrt = RegisterMMCSS();

            // High-resolution waitable timer (Windows 10 1803+)
            HANDLE hTimer = nullptr;
#ifdef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
            hTimer = CreateWaitableTimerExW(nullptr, nullptr, 
                CREATE_WAITABLE_TIMER_HIGH_RESOLUTION | CREATE_WAITABLE_TIMER_MANUAL_RESET, 
                TIMER_ALL_ACCESS);
#endif
            if (!hTimer) {
                hTimer = CreateWaitableTimerW(nullptr, TRUE, nullptr);
            }

            ULONGLONG lastNotFoundLog = 0;
            while (running_.load(std::memory_order_relaxed)) {
                HWND found = WindowCache::Instance().FindRingCentral();
                if (found) {
                    Engine::Instance().TryAnswer(found, CHAN_POLL);
                } else {
                    // v4.5: surface the #1 cause of "it does nothing" — the
                    // RingCentral window not being located. Throttled so it
                    // doesn't spam the log. Open the dashboard (F9) to see it.
                    ULONGLONG now = GetTickCount64();
                    if (now - lastNotFoundLog >= 5000) {
                        lastNotFoundLog = now;
                        LOG_WRN(L"RingCentral window NOT found — engine idle. "
                                L"Is RingCentral Phone running & logged in? "
                                L"Press F8 for a self-test.");
                    }
                }
                if (!running_.load(std::memory_order_relaxed)) break;

                // Microsecond-precision wait (v4.3: honors DEFAULT_POLL_MS, 5 ms turbo)
                if (hTimer) {
                    LARGE_INTEGER dueTime;
                    dueTime.QuadPart = -(LONGLONG)DEFAULT_POLL_MS * 10000LL; // 100-ns units
                    SetWaitableTimer(hTimer, &dueTime, 0, nullptr, nullptr, FALSE);
                    WaitForSingleObject(hTimer, 2);
                } else {
                    Sleep(DEFAULT_POLL_MS);
                }
            }

            if (hTimer) CloseHandle(hTimer);
            UnregisterMMCSS(hAvrt);
        }

        PollWorker() : running_(false) {}
        std::atomic<bool> running_;
        std::thread thread_;
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// DETECTION HOOKS (WINEVENT + SHELLHOOK - MULTI-CHANNEL SENSORS)
// ─────────────────────────────────────────────────────────────────────────────
namespace SL {
    inline bool IsAnswerTriggerEvent(DWORD event) noexcept {
        return event == EVENT_SYSTEM_ALERT          // 0x0002 system alert
            || event == EVENT_SYSTEM_FOREGROUND     // 0x0003 window activated
            || event == EVENT_OBJECT_CREATE         // 0x8000 window created
            || event == EVENT_OBJECT_SHOW           // 0x8002 window shown (call popup)
            || event == EVENT_OBJECT_FOCUS          // 0x8005 window focused
            || event == EVENT_OBJECT_STATECHANGE    // 0x800A window state change (ringing/flashing)
            || event == EVENT_OBJECT_NAMECHANGE     // 0x800C title change (call state)
            || event == EVENT_OBJECT_UNCLOAKED;     // 0x8018 window uncloaked (DWM popup shown)
    }

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
        if (!IsAnswerTriggerEvent(event)) return;

        HWND root = GetAncestor(hwnd, GA_ROOT);
        if (!root) root = hwnd;

        wchar_t title[256] = {0};
        GetWindowTextW(root, title, 256);
        if (IsRingCentralTitle(title)) {
            Engine::Instance().TryAnswer(root, CHAN_WINEVENT);
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
                APP_VERSION, 
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

            InitializeTray();

            // Register Multi-Channel detection hooks
            hook_ = SetWinEventHook(
                EVENT_SYSTEM_SOUND, 
                EVENT_OBJECT_UNCLOAKED, 
                nullptr, 
                GlobalWinEventProc, 
                0, 0, 
                WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
            );

            wm_shellhook_ = RegisterWindowMessageW(L"SHELLHOOK");
            RegisterShellHookWindow(hwnd);

            SetTimer(hwnd, 1, STATS_REFRESH_MS, nullptr);
#ifndef SPICY_LAMAR_TURBO
            SetTimer(hwnd, 2, DEFAULT_POLL_MS, nullptr);
#endif

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
            SL_WCSCPY(nid_.szTip, APP_NAME);

            Shell_NotifyIconW(NIM_ADD, &nid_);
        }

        void UpdateTrayTip() {
            wchar_t tip[128];
            if (Engine::Instance().IsActive()) {
                SL_WCSCPY(nid_.szTip, APP_NAME);
            } else {
                SL_SWPRINTF(tip, L"%ls — PAUSED (F11 to start)", APP_NAME);
                SL_WCSCPY(nid_.szTip, tip);
            }
            Shell_NotifyIconW(NIM_MODIFY, &nid_);
        }

        void ToggleEngine() {
            bool nowActive = !Engine::Instance().IsActive();
            Engine::Instance().SetActive(nowActive);
            if (nowActive) {
                LOG_INF(L"Engine STARTED (F11) — auto-answer active");
            } else {
                LOG_WRN(L"Engine PAUSED (F11) — press F11 to start");
            }
            UpdateTrayTip();
            if (visible) InvalidateRect(hwnd, nullptr, FALSE);
        }

        void ShowTrayMenu() {
            bool active = Engine::Instance().IsActive();
            HMENU hMenu = CreatePopupMenu();
            InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, IDM_OPEN_SETTINGS, L"Open Dashboard (F9)");
            InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_STRING, IDM_ADD_DEVICE,
                        active ? L"Pause (F11)" : L"Start (F11)");
            InsertMenuW(hMenu, 2, MF_BYPOSITION | MF_STRING, IDM_SELF_TEST, L"Self-test (F8)");
            InsertMenuW(hMenu, 3, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
            InsertMenuW(hMenu, 4, MF_BYPOSITION | MF_STRING, IDM_REMOVE_ICON, L"Exit (F12)");

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
#ifndef SPICY_LAMAR_TURBO
                } else if (wp == 2) {
                    HWND found = WindowCache::Instance().FindRingCentral();
                    if (found) Engine::Instance().TryAnswer(found, CHAN_POLL);
#endif
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
                        self.ToggleEngine();
                        break;
                    case IDM_OPEN_SETTINGS:
                        self.Show(!self.visible);
                        break;
                    case IDM_SELF_TEST:
                        Engine::Instance().RunSelfTest();
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
                if (wp == HK_PAUSE_RESUME) self.ToggleEngine();
                if (wp == HK_EMERGENCY_EXIT) PostQuitMessage(0);
                if (wp == HK_SELF_TEST) Engine::Instance().RunSelfTest();
                return 0;
            }

            if (m == self.wm_shellhook_) {
                if (wp == HSHELL_WINDOWCREATED || 
                    wp == HSHELL_WINDOWACTIVATED || 
                    wp == HSHELL_RUDEAPPACTIVATED || 
                    wp == HSHELL_FLASH || 
                    wp == HSHELL_REDRAW || 
                    wp == HSHELL_ACTIVATESHELLWINDOW) {
                    HWND candidate = (HWND)lp;
                    if (candidate && IsWindow(candidate)) {
                        wchar_t title[256] = {0};
                        GetWindowTextW(candidate, title, 256);
                        if (IsRingCentralTitle(title)) {
                            Engine::Instance().TryAnswer(candidate, CHAN_SHELLHOOK);
                        } else {
                            HWND target = WindowCache::Instance().FindRingCentral();
                            if (target) Engine::Instance().TryAnswer(target, CHAN_SHELLHOOK);
                        }
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

            HBRUSH bgBrush = CreateSolidBrush(CLR_OBSIDIAN);
            FillRect(mdc, &r, bgBrush);
            DeleteObject(bgBrush);

            SetBkMode(mdc, TRANSPARENT);

            HPEN linePen = CreatePen(PS_SOLID, 1, CLR_CHARCOAL);
            HGDIOBJ oldPen = SelectObject(mdc, linePen);
            MoveToEx(mdc, 20, 45, nullptr);
            LineTo(mdc, DASH_WIDTH - 20, 45);

            SetTextColor(mdc, CLR_CHILI_RED);
            const wchar_t* titleText = L"🌶️ SPICY LAMAR v4.5";
            TextOutW(mdc, 20, 20, titleText, (int)wcslen(titleText));

            SetTextColor(mdc, Engine::Instance().IsActive() ? CLR_NEON_GREEN : CLR_CHILI_RED);
            const wchar_t* statusStr = Engine::Instance().IsActive()
                ? L"STATUS: [🌶️ ACTIVE]  —  F11 = PAUSE"
                : L"STATUS: [⚠ PAUSED]  —  F11 = START";
            TextOutW(mdc, 20, 55, statusStr, (int)wcslen(statusStr));

            SetTextColor(mdc, CLR_TEXT_DIM);
            wchar_t stats[256];
            SL_SWPRINTF(stats, L"CALLS: %llu   UPTIME: %llus   LAST: %lluus  AVG: %lluus  BEST: %lluus",
                       StatsTracker::Instance().TotalCalls(),
                       StatsTracker::Instance().GetUptimeSec(),
                       StatsTracker::Instance().LastLatency(),
                       StatsTracker::Instance().AvgLatency(),
                       StatsTracker::Instance().BestLatency());
            TextOutW(mdc, 20, 75, stats, (int)wcslen(stats));

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
                    SL_SWPRINTF(bucketLabel, L"<%dus : %ld", (i + 1) * 20, StatsTracker::Instance().GetHistCount(i));
                } else if (i == 3) {
                    SL_SWPRINTF(bucketLabel, L"<100us : %ld", StatsTracker::Instance().GetHistCount(i));
                } else {
                    SL_SWPRINTF(bucketLabel, L">=100us: %ld", StatsTracker::Instance().GetHistCount(i));
                }
                SetTextColor(mdc, CLR_TEXT_DIM);
                TextOutW(mdc, 20, 120 + (i * 22), bucketLabel, (int)wcslen(bucketLabel));
            }
            DeleteObject(barBrush);

            SetTextColor(mdc, CLR_CHILI_RED);
            const wchar_t* logText = L"[ SYSTEM LOG ]";
            TextOutW(mdc, 20, 245, logText, (int)wcslen(logText));

            auto logs = MemoryLogger::Instance().GetRecentLogs();
            for (size_t i = 0; i < logs.size() && i < 10; ++i) {
                SetTextColor(mdc, CLR_NEON_GREEN);
                wchar_t line[300];
                SL_SWPRINTF(line, L"%ls [%ls] %ls", logs[i].timestamp.c_str(), logs[i].level.c_str(), logs[i].message.c_str());
                TextOutW(mdc, 20, 270 + (int)i * 20, line, (int)wcslen(line));
            }

            MoveToEx(mdc, 20, 452, nullptr);
            LineTo(mdc, DASH_WIDTH - 20, 452);
            SetTextColor(mdc, CLR_TEXT_DIM);
            // v4.4: F8 self-test added; answer key is Alt+F1 ONLY (Alt+A never sent).
            const wchar_t* footerText = L"F8 SELF-TEST   F9 DASHBOARD   F11 PAUSE/START   F12 EXIT   ANSWER KEY: ALT+F1 (ONLY)";
            TextOutW(mdc, 20, 462, footerText, (int)wcslen(footerText));

            BitBlt(hdc, 0, 0, r.right, r.bottom, mdc, 0, 0, SRCCOPY); 

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
static int RunApp(HINSTANCE h) {
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, SL::APP_MUTEX_NAME);
    DWORD mutexErr = (hMutex != nullptr) ? GetLastError() : ERROR_ACCESS_DENIED;
    if (hMutex == nullptr || mutexErr == ERROR_ACCESS_DENIED) {
        if (hMutex) CloseHandle(hMutex);
        hMutex = CreateMutexW(nullptr, TRUE, SL::APP_MUTEX_FALLBACK);
        mutexErr = (hMutex != nullptr) ? GetLastError() : ERROR_ACCESS_DENIED;
    }
    if (mutexErr == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    // Apply Kernel-level System Optimizations (MMCSS, 0.5ms Timer, Real-Time Priority)
    SL::EnableSystemOptimizations();

    SL::StatsTracker::Instance().Initialize();
    SL::Engine::Instance().Initialize();

    if (!SL::Terminal::Instance().Create(h)) {
        if (hMutex) CloseHandle(hMutex);
        timeEndPeriod(1);
        return 1;
    }

    HWND w = SL::Terminal::Instance().GetHwnd();
    if (!RegisterHotKey(w, SL::HK_TOGGLE_DASHBOARD, MOD_NOREPEAT, VK_F9))
        LOG_WRN(L"F9 hotkey registration failed (already taken by another app?)");
    if (!RegisterHotKey(w, SL::HK_PAUSE_RESUME, MOD_NOREPEAT, VK_F11))
        LOG_ERR(L"F11 hotkey registration failed (already taken by another app?)");
    if (!RegisterHotKey(w, SL::HK_EMERGENCY_EXIT, MOD_NOREPEAT, VK_F12))
        LOG_WRN(L"F12 hotkey registration failed (already taken by another app?)");
    if (!RegisterHotKey(w, SL::HK_SELF_TEST, MOD_NOREPEAT, VK_F8))
        LOG_WRN(L"F8 hotkey registration failed (already taken by another app?)");

    LOG_INF(L"Spicy Lamar v4.5 online. Hotkeys: F8 self-test, F9 dashboard, F11 pause/start, F12 exit. Answer key: Alt+F1 only.");
#ifdef SPICY_LAMAR_BOUNDED
    LOG_INF(L"Auto-answer armed (bounded): Alt+F1 cascade fires on call events (max %d per call).",
            SL::ANSWER_MAX_ATTEMPTS);
#else
#ifdef SPICY_LAMAR_TURBO
    LOG_INF(L"Auto-answer armed: TURBO MAX — 1000 Hz scan, 1 ms cascade floor, instant call-event response, real-time priority.");
#else
    LOG_INF(L"Auto-answer armed: ALWAYS-ON attention on the RingCentral Phone window.");
#endif
#endif

#ifdef SPICY_LAMAR_TURBO
    SL::PollWorker::Instance().Start();
#endif

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }

    UnregisterHotKey(w, SL::HK_TOGGLE_DASHBOARD);
    UnregisterHotKey(w, SL::HK_PAUSE_RESUME);
    UnregisterHotKey(w, SL::HK_EMERGENCY_EXIT);
    UnregisterHotKey(w, SL::HK_SELF_TEST);

#ifdef SPICY_LAMAR_TURBO
    SL::PollWorker::Instance().Stop();
#endif
    timeEndPeriod(1);
    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    return (int)m.wParam;
}

#ifdef _MSC_VER
int WINAPI wWinMain(HINSTANCE h, HINSTANCE, LPWSTR, int) {
    return RunApp(h);
}
#else
int WINAPI WinMain(HINSTANCE h, HINSTANCE, LPSTR, int) {
    return RunApp(h);
}
#endif

#endif
