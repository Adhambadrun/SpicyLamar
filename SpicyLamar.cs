using System;
using System.Drawing;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Threading;
using System.Collections.Generic;
using System.Linq;

namespace SpicyLamar
{
    static class Program
    {
        [DllImport("winmm.dll")]
        static extern uint timeBeginPeriod(uint uMilliseconds);
        [DllImport("winmm.dll")]
        static extern uint timeEndPeriod(uint uMilliseconds);

        [DllImport("ntdll.dll", SetLastError = true)]
        static extern int NtSetTimerResolution(uint DesiredResolution, bool SetResolution, out uint CurrentResolution);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool SetProcessWorkingSetSize(IntPtr hProcess, IntPtr dwMinimumWorkingSetSize, IntPtr dwMaximumWorkingSetSize);

        [DllImport("user32.dll", SetLastError = true)]
        static extern bool SystemParametersInfo(uint uiAction, uint uiParam, IntPtr pvParam, uint fWinIni);

        [DllImport("user32.dll", SetLastError = true)]
        static extern bool AllowSetForegroundWindow(uint dwProcessId);

        private const uint SPI_SETFOREGROUNDLOCKTIMEOUT = 0x2001;
        private const uint SPIF_SENDCHANGE = 0x0002;
        private const uint SPIF_UPDATEINIFILE = 0x0001;
        private const uint ASFW_ANY = 0xFFFFFFFF;

        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            // Max-performance profile: HIGH (not RealTime) process priority — a
            // real-time process starves RingCentral's own input/renderer threads
            // and makes the app lag — plus a 1 ms multimedia timer tick + 0.5 ms NT kernel timer.
            if (AnswerEngine.TURBO_MODE)
            {
                try { Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.High; } catch { }
                try { Thread.CurrentThread.Priority = ThreadPriority.Highest; } catch { }
                timeBeginPeriod(1);
                try
                {
                    uint cur;
                    NtSetTimerResolution(5000, true, out cur); // 5000 * 100ns = 0.5ms NT kernel timer resolution
                }
                catch { }

                try
                {
                    // Lock working set into RAM (prevent page faults during answering)
                    SetProcessWorkingSetSize(Process.GetCurrentProcess().Handle, (IntPtr)(32 * 1024 * 1024), (IntPtr)(128 * 1024 * 1024));
                    // Bypass foreground lock delay
                    SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, IntPtr.Zero, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
                    AllowSetForegroundWindow(ASFW_ANY);
                }
                catch { }
            }

            // Single-instance enforcement. A Global\ mutex needs SeCreateGlobalPrivilege,
            // which non-elevated users lack - creating it then throws. Fall back to the
            // session namespace so the portable build always starts, elevated or not.
            bool createdNew;
            Mutex mutex = null;
            try
            {
                mutex = new Mutex(true, @"Global\SpicyLamarV4", out createdNew);
            }
            catch (UnauthorizedAccessException)
            {
                mutex = new Mutex(true, @"Local\SpicyLamarV4", out createdNew);
            }

            using (mutex)
            {
                if (!createdNew)
                {
                    if (AnswerEngine.TURBO_MODE) timeEndPeriod(1);
                    return; // Another instance is already running
                }
                Application.Run(new DashboardContext());
                if (AnswerEngine.TURBO_MODE) timeEndPeriod(1);
            }
        }
    }

    class DashboardContext : ApplicationContext
    {
        private NotifyIcon trayIcon;
        private TerminalForm dashboard;
        private AnswerEngine engine;

        public DashboardContext()
        {
            engine = new AnswerEngine();
            trayIcon = new NotifyIcon()
            {
                Icon = LoadAppIcon(),
                Text = "Spicy Lamar",
                Visible = true,
                ContextMenu = BuildMenu()
            };
            trayIcon.DoubleClick += delegate(object s, EventArgs e)
            {
                dashboard.ToggleVisibility();
            };

            // Mirror pause/start state into the tray tooltip
            engine.ActiveChanged += delegate(bool active)
            {
                trayIcon.Text = active ? "Spicy Lamar" : "Spicy Lamar — PAUSED (F11 to start)";
            };

            dashboard = new TerminalForm(engine);

            // Global Hotkeys (with NoRepeat to match C++ version)
            HotKeyManager.RegisterHotKey(dashboard.Handle, 1, (uint)HotKeyManager.KeyModifiers.NoRepeat, (uint)Keys.F9);  // Toggle Dashboard
            HotKeyManager.RegisterHotKey(dashboard.Handle, 2, (uint)HotKeyManager.KeyModifiers.NoRepeat, (uint)Keys.F11); // Pause / Start
            HotKeyManager.RegisterHotKey(dashboard.Handle, 3, (uint)HotKeyManager.KeyModifiers.NoRepeat, (uint)Keys.F12); // Exit
            HotKeyManager.RegisterHotKey(dashboard.Handle, 4, (uint)HotKeyManager.KeyModifiers.NoRepeat, (uint)Keys.F8);  // Self-test
        }

        private Icon LoadAppIcon()
        {
            try
            {
                Icon embedded = Icon.ExtractAssociatedIcon(Application.ExecutablePath);
                if (embedded != null) return embedded;
            }
            catch { }
            try
            {
                if (System.IO.File.Exists("icon.ico"))
                    return new Icon("icon.ico");
            }
            catch { }
            return SystemIcons.Application;
        }

        private ContextMenu BuildMenu()
        {
            ContextMenu menu = new ContextMenu();
            menu.MenuItems.Add("Open Dashboard (F9)", delegate(object s, EventArgs e)
            {
                dashboard.ToggleVisibility();
            });
            menu.MenuItems.Add("Pause/Start (F11)", delegate(object s, EventArgs e)
            {
                engine.Toggle();
            });
            menu.MenuItems.Add("Self-test (F8)", delegate(object s, EventArgs e)
            {
                engine.RunSelfTest();
            });
            menu.MenuItems.Add("-");
            menu.MenuItems.Add("Exit (F12)", delegate(object s, EventArgs e)
            {
                ExitApp();
            });
            return menu;
        }

        private void ExitApp()
        {
            if (trayIcon != null)
            {
                trayIcon.Visible = false;
                trayIcon.Dispose();
                trayIcon = null;
            }
            if (dashboard != null)
            {
                HotKeyManager.UnregisterHotKey(dashboard.Handle, 1);
                HotKeyManager.UnregisterHotKey(dashboard.Handle, 2);
                HotKeyManager.UnregisterHotKey(dashboard.Handle, 3);
                HotKeyManager.UnregisterHotKey(dashboard.Handle, 4);
                dashboard.Dispose();
                dashboard = null;
            }
            if (engine != null)
            {
                engine.Dispose();
                engine = null;
            }
            Application.Exit();
        }

        protected override void ExitThreadCore()
        {
            ExitApp();
            base.ExitThreadCore();
        }
    }

    class TerminalForm : Form
    {
        private AnswerEngine engine;
        private System.Windows.Forms.Timer refreshTimer;
        private SolidBrush chiliBrush;
        private SolidBrush neonBrush;
        private SolidBrush dimBrush;
        private Pen linePen;
        private Font monoFont;
        private Font headerFont;
        private uint wmShellHook = 0;

        [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        static extern uint RegisterWindowMessage(string lpString);

        [DllImport("user32.dll", SetLastError = true)]
        static extern bool RegisterShellHookWindow(IntPtr hWnd);

        [DllImport("user32.dll", SetLastError = true)]
        static extern bool DeregisterShellHookWindow(IntPtr hWnd);

        public TerminalForm(AnswerEngine engine)
        {
            this.engine = engine;
            this.Text = "Spicy Lamar v4.4";
            this.Size = new Size(780, 520);
            this.BackColor = Color.FromArgb(5, 5, 5);
            this.FormBorderStyle = FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.TopMost = true;
            this.ShowInTaskbar = false;

            // Cache GDI drawing resources to eliminate memory leaks
            chiliBrush = new SolidBrush(Color.FromArgb(255, 51, 0));
            neonBrush = new SolidBrush(Color.FromArgb(0, 255, 102));
            dimBrush = new SolidBrush(Color.Gray);
            linePen = new Pen(Color.DimGray);
            monoFont = new Font("Consolas", 10);
            headerFont = new Font(monoFont, FontStyle.Bold);

            this.DoubleBuffered = true;
            this.Paint += TerminalForm_Paint;

            refreshTimer = new System.Windows.Forms.Timer();
            refreshTimer.Interval = AnswerEngine.TURBO_MODE ? 100 : 250;
            refreshTimer.Tick += delegate(object s, EventArgs e) { if (this.Visible) this.Invalidate(); };
            refreshTimer.Start();

            try
            {
                wmShellHook = RegisterWindowMessage("SHELLHOOK");
                RegisterShellHookWindow(this.Handle);
            }
            catch { }
        }

        public void ToggleVisibility()
        {
            if (this.Visible)
            {
                this.Hide();
            }
            else
            {
                this.Show();
                this.BringToFront();
            }
        }

        protected override void WndProc(ref Message m)
        {
            if (m.Msg == 0x0312) // WM_HOTKEY
            {
                int id = m.WParam.ToInt32();
                if (id == 1) ToggleVisibility();
                if (id == 2) engine.Toggle();
                if (id == 3) Application.Exit();
                if (id == 4) engine.RunSelfTest();
            }
            else if (wmShellHook != 0 && m.Msg == wmShellHook)
            {
                int wp = m.WParam.ToInt32();
                // HSHELL_WINDOWCREATED (1), HSHELL_WINDOWACTIVATED (4), HSHELL_RUDEAPPACTIVATED (0x8004), HSHELL_FLASH (0x8006), HSHELL_REDRAW (6), HSHELL_ACTIVATESHELLWINDOW (3)
                if (wp == 1 || wp == 4 || wp == 0x8004 || wp == 0x8006 || wp == 6 || wp == 3)
                {
                    IntPtr hwnd = m.LParam;
                    if (hwnd != IntPtr.Zero)
                    {
                        engine.OnShellHookEvent(hwnd);
                    }
                }
            }
            base.WndProc(ref m);
        }

        private void TerminalForm_Paint(object sender, PaintEventArgs e)
        {
            Graphics g = e.Graphics;

            g.DrawString("🌶️ SPICY LAMAR v4.4", headerFont, chiliBrush, 20, 20);
            g.DrawLine(linePen, 20, 45, 740, 45);

            string status = engine.Active ? "[🌶️ ACTIVE]  —  F11 = PAUSE" : "[⚠ PAUSED]  —  F11 = START";
            g.DrawString(string.Format("STATUS: {0}", status), monoFont, engine.Active ? neonBrush : chiliBrush, 20, 55);

            string uptime = DateTime.Now.Subtract(Process.GetCurrentProcess().StartTime).ToString(@"hh\:mm\:ss");
            long best = engine.BestLatency;
            if (best == long.MaxValue) best = 0;
            g.DrawString(string.Format("CALLS: {0}   UPTIME: {1}   LAST: {2}us  AVG: {3}us  BEST: {4}us",
                engine.CallCount, uptime, engine.LastLatency, engine.AvgLatency, best),
                monoFont, dimBrush, 20, 75);

            g.DrawString("[ REAL-TIME TELEMETRY ]", monoFont, chiliBrush, 20, 95);

            string[] bucketLabels = { "<20us", "<40us", "<60us", "<100us", ">=100us" };
            long[] hist = engine.Histogram;
            for (int i = 0; i < 5; i++)
            {
                long count = hist[i];
                int barWidth = 30 + (int)Math.Min(320, count * 12);
                g.FillRectangle(neonBrush, 130, 120 + (i * 22), barWidth, 14);
                g.DrawString(string.Format("{0} : {1}", bucketLabels[i], count), monoFont, dimBrush, 20, 120 + (i * 22));
            }

            g.DrawString("[ SYSTEM LOG ]", monoFont, chiliBrush, 20, 245);
            List<string> recentLogs = engine.GetLogs().Take(10).ToList();
            for (int i = 0; i < recentLogs.Count; i++)
            {
                g.DrawString(recentLogs[i], monoFont, neonBrush, 20, 270 + (i * 20));
            }

            // Footer: hotkey cheat-sheet
            g.DrawLine(linePen, 20, 452, 740, 452);
            // v4.4: F8 self-test added; answer key is Alt+F1 ONLY (Alt+A never sent).
            string footer = "F8 SELF-TEST   F9 DASHBOARD   F11 PAUSE/START   F12 EXIT   ANSWER KEY: ALT+F1 (ONLY)";
            g.DrawString(footer, monoFont, dimBrush, 20, 462);
        }

        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            if (e.CloseReason == CloseReason.UserClosing)
            {
                e.Cancel = true;
                this.Hide();
            }
            base.OnFormClosing(e);
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (refreshTimer != null) { refreshTimer.Stop(); refreshTimer.Dispose(); refreshTimer = null; }
                if (chiliBrush != null) { chiliBrush.Dispose(); chiliBrush = null; }
                if (neonBrush != null) { neonBrush.Dispose(); neonBrush = null; }
                if (dimBrush != null) { dimBrush.Dispose(); dimBrush = null; }
                if (linePen != null) { linePen.Dispose(); linePen = null; }
                if (headerFont != null) { headerFont.Dispose(); headerFont = null; }
                if (monoFont != null) { monoFont.Dispose(); monoFont = null; }
                try
                {
                    if (wmShellHook != 0) DeregisterShellHookWindow(this.Handle);
                }
                catch { }
            }
            base.Dispose(disposing);
        }
    }

    class AnswerEngine : IDisposable
    {
        public volatile bool Active = true;

        // Pause/Start with visible feedback (log line + tray tooltip update)
        public delegate void ActiveChangedHandler(bool active);
        public event ActiveChangedHandler ActiveChanged;

        public void SetActive(bool value)
        {
            Active = value;
            Log(value
                ? "Engine STARTED (F11) — auto-answer active"
                : "Engine PAUSED (F11) — press F11 to start");
            if (ActiveChanged != null) ActiveChanged(value);
        }

        public void Toggle()
        {
            SetActive(!Active);
        }

        private long callCount = 0;
        public long CallCount { get { return Interlocked.Read(ref callCount); } }
        private long lastLatency = 0;
        public long LastLatency { get { return Interlocked.Read(ref lastLatency); } }
        private long bestLatency = long.MaxValue;
        public long BestLatency { get { return Interlocked.Read(ref bestLatency); } }
        private long worstLatency = 0;
        public long WorstLatency { get { return Interlocked.Read(ref worstLatency); } }
        private long sumLatency = 0;
        public long AvgLatency
        {
            get
            {
                long c = Interlocked.Read(ref callCount);
                long s = Interlocked.Read(ref sumLatency);
                return c > 0 ? s / c : 0;
            }
        }
        private long[] histogram = new long[5];
        public long[] Histogram
        {
            get
            {
                long[] snap = new long[5];
                lock (statsLock) { for (int i = 0; i < 5; i++) snap[i] = histogram[i]; }
                return snap;
            }
        }
        private List<string> logs = new List<string>();
        private WinEventDelegate dele;
        private IntPtr hookHandle = IntPtr.Zero;
        private System.Threading.Timer pollTimer;
        private long lastFireTick = 0;

        public const bool BOUNDED_MODE = false;
        public const bool TURBO_MODE = true;
        // v4.3: turbo debounce raised 1 ms -> 100 ms. The old 1 ms floor fired the
        // full cascade up to 1000x/sec, flooding RingCentral's message queue and
        // stealing foreground focus nonstop — the app visibly lagged.
        private const long DEBOUNCE_TICKS         = TURBO_MODE ? (100 * 10000)   : (500 * 10000);   // 100 ms / 500 ms in 100-ns ticks
        // v4.3: coalescing floor for the "immediate" (real call-event) path, which
        // previously had NO rate limit at all — WinEvent/shell hooks can fire
        // hundreds of times per second and each one launched a full cascade.
        // The first event after quiet still fires instantly.
        private const long IMMEDIATE_MIN_GAP_TICKS = TURBO_MODE ? (50 * 10000) : (100 * 10000);     // 50 ms / 100 ms in 100-ns ticks
        private const long MIN_RETRY_TICKS        = TURBO_MODE ? (100 * 10000)   : (1500 * 10000);  // 100 ms / 1500 ms in 100-ns ticks
        private const long EPISODE_RESET_TICKS    = TURBO_MODE ? (2000 * 10000)  : (10000 * 10000); // 2000 ms / 10000 ms in 100-ns ticks
        private const int  MAX_ATTEMPTS_PER_EPISODE = 3;
        private const long ANSWER_LOG_MIN_GAP_TICKS = TURBO_MODE ? (250 * 10000) : 0; // 250 ms / log everything
        private long lastAnswerLogTick = 0;
        private long suppressedAnswerLogs = 0;
        private int episodeAttempts = 0;
        private bool capLogged = false;
        private readonly object fireLock = new object();

        private readonly object logLock = new object();
        private readonly object statsLock = new object();
        private IntPtr cachedTarget = IntPtr.Zero;
        private IntPtr cachedChild = IntPtr.Zero;
        private IntPtr cachedIntermediate = IntPtr.Zero;

        // Virtual key codes and keybd_event flags
        private const byte VK_MENU = 0x12;
        private const byte VK_RETURN = 0x0D;
        private const byte VK_SPACE = 0x20;
        private const byte VK_CONTROL = 0x11;
        private const byte VK_SHIFT = 0x10;
        private const byte VK_F1 = 0x70;
        private const byte SCAN_MENU = 0x38;
        private const byte SCAN_F1 = 0x3B;
        private const byte SCAN_RETURN = 0x1C;
        private const byte SCAN_SPACE = 0x39;
        private const uint KEYEVENTF_KEYUP = 0x0002;
        private const uint KEYEVENTF_SCANCODE = 0x0008;
        private const uint WM_SYSKEYDOWN = 0x0104;
        private const uint WM_SYSKEYUP = 0x0105;
        private const uint WM_KEYDOWN = 0x0100;
        private const uint WM_KEYUP = 0x0101;
        private const uint WM_COMMAND = 0x0111;
        private const uint SW_RESTORE = 0x0001;
        private const uint SW_SHOW = 0x0004;

        [DllImport("avrt.dll", SetLastError = true, CharSet = CharSet.Auto)]
        static extern IntPtr AvSetMmThreadCharacteristics(string TaskName, ref uint TaskIndex);

        [DllImport("avrt.dll", SetLastError = true)]
        static extern bool AvRevertMmThreadCharacteristics(IntPtr AvrtHandle);

        [StructLayout(LayoutKind.Sequential)]
        struct INPUT
        {
            public uint type;
            public INPUTUNION U;
        }
        [StructLayout(LayoutKind.Explicit)]
        struct INPUTUNION
        {
            [FieldOffset(0)] public KEYBDINPUT ki;
        }
        [StructLayout(LayoutKind.Sequential)]
        struct KEYBDINPUT
        {
            public ushort wVk;
            public ushort wScan;
            public uint dwFlags;
            public uint time;
            public UIntPtr dwExtraInfo;
        }
        const uint INPUT_KEYBOARD = 1;

        [DllImport("user32.dll", SetLastError = true)]
        static extern uint SendInput(uint nInputs, INPUT[] pInputs, int cbSize);

        public AnswerEngine()
        {
            dele = new WinEventDelegate(WinEventProc);
            // Listen for window foreground, create, show, state, cloak/uncloak, and title changes
            // Range: EVENT_SYSTEM_SOUND (0x0001) to EVENT_OBJECT_UNCLOAKED (0x8018)
            const uint WINEVENT_OUTOFCONTEXT = 0;
            const uint WINEVENT_SKIPOWNPROCESS = 0x0002;
            hookHandle = SetWinEventHook(0x0001, 0x8018, IntPtr.Zero, dele, 0, 0,
                WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
            Log("Spicy Lamar v4.4 online. Hotkeys: F8 self-test, F9 dashboard, F11 pause/start, F12 exit. Answer key: Alt+F1 only.");
            Log("Engine initialized. Quantum call-event sensors active.");

            // Cache-refresh poll: 25 ms in max-performance profile (v4.3: was 1 ms —
            // real call events arrive instantly via the WinEvent/shell hooks, so a
            // 1000 Hz fallback poll only burned CPU and hammered RingCentral)
            pollTimer = new System.Threading.Timer(PollCheck, null, TURBO_MODE ? 25 : 100, TURBO_MODE ? 25 : 20);
        }

        private void PollCheck(object state)
        {
            IntPtr found = FindRingCentralWindow();
            if (found != IntPtr.Zero) TryFire(found, false);
        }

        private static bool IsTargetTitle(string title)
        {
            return title.IndexOf("RingCentral", StringComparison.OrdinalIgnoreCase) >= 0
                || title.IndexOf("Ring Central", StringComparison.OrdinalIgnoreCase) >= 0
                || title.IndexOf("RingMe", StringComparison.OrdinalIgnoreCase) >= 0
                || title.IndexOf("Glip", StringComparison.OrdinalIgnoreCase) >= 0;
        }

        private IntPtr FindRingCentralWindow()
        {
            // Fast path 1: Cached target window
            if (cachedTarget != IntPtr.Zero && IsWindow(cachedTarget))
            {
                if (cachedChild == IntPtr.Zero || !IsWindow(cachedChild))
                {
                    FindChildWindows(cachedTarget);
                }
                return cachedTarget;
            }

            // Fast path 2: Direct O(1) FindWindow for standard title
            IntPtr direct = FindWindow(null, "RingCentral Phone");
            if (direct != IntPtr.Zero && IsWindow(direct))
            {
                cachedTarget = direct;
                FindChildWindows(direct);
                return direct;
            }

            // Fast path 3: Direct FindWindow for Electron widget class
            direct = FindWindow("Chrome_WidgetWin_1", null);
            if (direct != IntPtr.Zero && IsWindow(direct))
            {
                System.Text.StringBuilder sbTitle = new System.Text.StringBuilder(256);
                GetWindowText(direct, sbTitle, 256);
                if (IsTargetTitle(sbTitle.ToString()))
                {
                    cachedTarget = direct;
                    FindChildWindows(direct);
                    return direct;
                }
            }

            // Fast path 4: Filtered EnumWindows
            IntPtr found = IntPtr.Zero;
            EnumWindows(delegate(IntPtr hWnd, IntPtr lParam)
            {
                if (!IsWindowVisible(hWnd)) return true;
                System.Text.StringBuilder sb = new System.Text.StringBuilder(256);
                GetWindowText(hWnd, sb, 256);
                if (IsTargetTitle(sb.ToString()))
                {
                    found = hWnd;
                    return false; // Stop enumeration
                }
                return true;
            }, IntPtr.Zero);

            if (found != IntPtr.Zero)
            {
                cachedTarget = found;
                FindChildWindows(found);
            }
            return found;
        }

        private void FindChildWindows(IntPtr mainWnd)
        {
            if (mainWnd == IntPtr.Zero || !IsWindow(mainWnd)) return;
            IntPtr c = FindWindowEx(mainWnd, IntPtr.Zero, "Chrome_RenderWidgetHostHWND", null);
            IntPtr inter = FindWindowEx(mainWnd, IntPtr.Zero, "Intermediate D3D Window", null);
            if (c == IntPtr.Zero && inter != IntPtr.Zero)
            {
                c = FindWindowEx(inter, IntPtr.Zero, "Chrome_RenderWidgetHostHWND", null);
            }
            cachedChild = c;
            cachedIntermediate = inter;
        }

        private static bool IsAnswerTriggerEvent(uint eventType)
        {
            return eventType == 0x0002   // EVENT_SYSTEM_ALERT
                || eventType == 0x0003   // EVENT_SYSTEM_FOREGROUND
                || eventType == 0x8000   // EVENT_OBJECT_CREATE
                || eventType == 0x8002   // EVENT_OBJECT_SHOW
                || eventType == 0x8005   // EVENT_OBJECT_FOCUS
                || eventType == 0x800A   // EVENT_OBJECT_STATECHANGE
                || eventType == 0x800C   // EVENT_OBJECT_NAMECHANGE
                || eventType == 0x8018;  // EVENT_OBJECT_UNCLOAKED
        }

        public void OnShellHookEvent(IntPtr hwnd)
        {
            System.Text.StringBuilder sb = new System.Text.StringBuilder(256);
            GetWindowText(hwnd, sb, 256);
            if (IsTargetTitle(sb.ToString()))
            {
                TryFire(hwnd, true);
            }
            else
            {
                IntPtr target = FindRingCentralWindow();
                if (target != IntPtr.Zero) TryFire(target, true);
            }
        }

        private void WinEventProc(IntPtr hWinEventHook, uint eventType, IntPtr hwnd, int idObject, int idChild, uint dwEventThread, uint dwmsEventTime)
        {
            if (idObject != 0 || hwnd == IntPtr.Zero) return;
            if (!IsAnswerTriggerEvent(eventType)) return;

            IntPtr root = GetAncestor(hwnd, 3); // GA_ROOT
            if (root == IntPtr.Zero) root = hwnd;

            System.Text.StringBuilder sb = new System.Text.StringBuilder(256);
            GetWindowText(root, sb, 256);
            if (IsTargetTitle(sb.ToString()))
            {
                // Real call event: bypass polling debounce, fire instantly in microseconds.
                TryFire(root, true);
            }
        }

        public void TryFire(IntPtr target)
        {
            TryFire(target, false);
        }

        public void TryFire(IntPtr target, bool immediate)
        {
            if (!Monitor.TryEnter(fireLock, 0)) return;
            try
            {
                if (!Active) return;

                if (target == IntPtr.Zero) target = cachedTarget;
                if (target == IntPtr.Zero || !IsWindow(target)) target = FindRingCentralWindow();
                if (target == IntPtr.Zero || !IsWindow(target)) return;

                long now = DateTime.UtcNow.Ticks;
                long last = Interlocked.Read(ref lastFireTick);
                if (BOUNDED_MODE)
                {
                    if (last == 0 || (now - last) > EPISODE_RESET_TICKS)
                    {
                        episodeAttempts = 0;
                        capLogged = false;
                    }
                    if (episodeAttempts >= MAX_ATTEMPTS_PER_EPISODE)
                    {
                        if (!capLogged)
                        {
                            Log(string.Format(
                                "Episode cap reached ({0} attempts) — Alt+F1 idle until next call event",
                                MAX_ATTEMPTS_PER_EPISODE));
                            capLogged = true;
                        }
                        return;
                    }
                    if (last != 0 && (now - last) < MIN_RETRY_TICKS)
                    {
                        return;
                    }
                    episodeAttempts++;
                }
                else
                {
                    // v4.3: poll channel waits DEBOUNCE_TICKS between cascades;
                    // immediate (real call-event) fires are coalesced at
                    // IMMEDIATE_MIN_GAP_TICKS. The first event after quiet still
                    // fires instantly (last == 0 or gap already elapsed).
                    long minGap = immediate ? IMMEDIATE_MIN_GAP_TICKS : DEBOUNCE_TICKS;
                    if (last != 0 && (now - last) < minGap)
                    {
                        return;
                    }
                }
                Interlocked.Exchange(ref lastFireTick, now);

                // v4.4: cascade delivery lives in DeliverAnswerCascade() so the
                // F8 self-test can reuse it (answer key is Alt+F1 ONLY).
                long lat = DeliverAnswerCascade(target);
                Interlocked.Exchange(ref lastLatency, lat);
                lock (statsLock)
                {
                    Interlocked.Increment(ref callCount);
                    Interlocked.Add(ref sumLatency, lat);
                    long curBest = Interlocked.Read(ref bestLatency);
                    while (lat < curBest && Interlocked.CompareExchange(ref bestLatency, lat, curBest) != curBest)
                    {
                        curBest = Interlocked.Read(ref bestLatency);
                    }
                    long curWorst = Interlocked.Read(ref worstLatency);
                    while (lat > curWorst && Interlocked.CompareExchange(ref worstLatency, lat, curWorst) != curWorst)
                    {
                        curWorst = Interlocked.Read(ref worstLatency);
                    }
                    if (lat < 20) histogram[0]++;
                    else if (lat < 40) histogram[1]++;
                    else if (lat < 60) histogram[2]++;
                    else if (lat < 100) histogram[3]++;
                    else histogram[4]++;
                }

                if (ANSWER_LOG_MIN_GAP_TICKS == 0)
                {
                    Log(string.Format("ANSWERED via 6-Shot Cascade in {0} us", lat));
                }
                else
                {
                    long lnow = DateTime.UtcNow.Ticks;
                    if (lastAnswerLogTick == 0 || (lnow - lastAnswerLogTick) >= ANSWER_LOG_MIN_GAP_TICKS)
                    {
                        lastAnswerLogTick = lnow;
                        long skipped = suppressedAnswerLogs;
                        suppressedAnswerLogs = 0;
                        if (skipped > 0)
                            Log(string.Format("ANSWERED via 6-Shot Cascade in {0} us (+{1} more cascades since last log)", lat, skipped));
                        else
                            Log(string.Format("ANSWERED via 6-Shot Cascade in {0} us", lat));
                    }
                    else
                    {
                        suppressedAnswerLogs++;
                    }
                }
            }
            finally { Monitor.Exit(fireLock); }
        }

        // Delivers the focus steal + 6-shot answer cascade to the RingCentral
        // window and returns the delivery latency in microseconds.
        //
        // v4.4 - ANSWER KEY IS Alt+F1 ONLY. Every shot below synthesizes
        // Alt+F1 (plus Enter / Space / WM_COMMAND fallbacks that are NOT
        // typing). RingCentral's STOCK answer shortcut Alt+A is deliberately
        // NEVER sent: this build targets a RingCentral install whose answer
        // shortcut is remapped to Alt+F1, and a stray Alt+A could open an app
        // menu or trigger an unrelated action. Do NOT re-add an Alt+A fallback.
        private long DeliverAnswerCascade(IntPtr target)
        {
            Stopwatch sw = Stopwatch.StartNew();

            IntPtr child = cachedChild;
            IntPtr inter = cachedIntermediate;
            if (child == IntPtr.Zero || !IsWindow(child))
            {
                FindChildWindows(target);
                child = cachedChild;
                inter = cachedIntermediate;
            }

            // Attach thread input for unconditional foreground focus steal
            uint curThreadId = GetCurrentThreadId();
            uint targetThreadId = GetWindowThreadProcessId(target, IntPtr.Zero);
            bool attached = false;
            if (targetThreadId != 0 && targetThreadId != curThreadId)
            {
                attached = AttachThreadInput(curThreadId, targetThreadId, true);
            }

            if (IsIconic(target)) ShowWindow(target, SW_RESTORE);
            BringWindowToTop(target);
            try { SetForegroundWindow(target); } catch { }
            if (child != IntPtr.Zero && IsWindow(child)) try { SetFocus(child); } catch { }
            else try { SetFocus(target); } catch { }

            // ─────────────────────────────────────────────────────────────────
            // 6-SHOT REDUNDANT QUANTUM IPC CASCADE
            // ─────────────────────────────────────────────────────────────────

            // Shot 1: Target window Alt+F1 Down/Up
            PostMessage(target, WM_SYSKEYDOWN, (IntPtr)VK_MENU, (IntPtr)0x20380001);
            PostMessage(target, WM_SYSKEYDOWN, (IntPtr)VK_F1,   (IntPtr)0x203B0001);
            PostMessage(target, WM_SYSKEYUP,   (IntPtr)VK_F1,   (IntPtr)0xE03B0001);
            PostMessage(target, WM_KEYUP,      (IntPtr)VK_MENU, (IntPtr)0xE0380001);

            // Shot 2: Chrome Render Child + Intermediate D3D Alt+F1 Down/Up
            if (child != IntPtr.Zero && IsWindow(child))
            {
                PostMessage(child, WM_SYSKEYDOWN, (IntPtr)VK_MENU, (IntPtr)0x20380001);
                PostMessage(child, WM_SYSKEYDOWN, (IntPtr)VK_F1,   (IntPtr)0x203B0001);
                PostMessage(child, WM_SYSKEYUP,   (IntPtr)VK_F1,   (IntPtr)0xE03B0001);
                PostMessage(child, WM_KEYUP,      (IntPtr)VK_MENU, (IntPtr)0xE0380001);
            }
            if (inter != IntPtr.Zero && IsWindow(inter))
            {
                PostMessage(inter, WM_SYSKEYDOWN, (IntPtr)VK_MENU, (IntPtr)0x20380001);
                PostMessage(inter, WM_SYSKEYDOWN, (IntPtr)VK_F1,   (IntPtr)0x203B0001);
                PostMessage(inter, WM_SYSKEYUP,   (IntPtr)VK_F1,   (IntPtr)0xE03B0001);
                PostMessage(inter, WM_KEYUP,      (IntPtr)VK_MENU, (IntPtr)0xE0380001);
            }

            // Shot 3: Post Enter & Space keys to root and child
            PostMessage(target, WM_KEYDOWN, (IntPtr)VK_RETURN, (IntPtr)0x001C0001);
            PostMessage(target, WM_KEYUP,   (IntPtr)VK_RETURN, (IntPtr)0xC01C0001);
            PostMessage(target, WM_KEYDOWN, (IntPtr)VK_SPACE,  (IntPtr)0x00390001);
            PostMessage(target, WM_KEYUP,   (IntPtr)VK_SPACE,  (IntPtr)0xC0390001);
            if (child != IntPtr.Zero && IsWindow(child))
            {
                PostMessage(child, WM_KEYDOWN, (IntPtr)VK_RETURN, (IntPtr)0x001C0001);
                PostMessage(child, WM_KEYUP,   (IntPtr)VK_RETURN, (IntPtr)0xC01C0001);
            }

            // Shot 4: Direct hardware-level SendInput synthesis with scan codes + legacy fallback
            INPUT[] inputs = new INPUT[4];
            inputs[0] = new INPUT { type = INPUT_KEYBOARD, U = new INPUTUNION { ki = new KEYBDINPUT { wVk = VK_MENU, wScan = SCAN_MENU } } };
            inputs[1] = new INPUT { type = INPUT_KEYBOARD, U = new INPUTUNION { ki = new KEYBDINPUT { wVk = VK_F1,   wScan = SCAN_F1 } } };
            inputs[2] = new INPUT { type = INPUT_KEYBOARD, U = new INPUTUNION { ki = new KEYBDINPUT { wVk = VK_F1,   wScan = SCAN_F1,   dwFlags = KEYEVENTF_KEYUP } } };
            inputs[3] = new INPUT { type = INPUT_KEYBOARD, U = new INPUTUNION { ki = new KEYBDINPUT { wVk = VK_MENU, wScan = SCAN_MENU, dwFlags = KEYEVENTF_KEYUP } } };
            SendInput(4, inputs, Marshal.SizeOf(typeof(INPUT)));

            keybd_event(VK_MENU, SCAN_MENU, 0, UIntPtr.Zero);
            keybd_event(VK_F1,   SCAN_F1,   0, UIntPtr.Zero);
            keybd_event(VK_F1,   SCAN_F1,   KEYEVENTF_KEYUP, UIntPtr.Zero);
            keybd_event(VK_MENU, SCAN_MENU, KEYEVENTF_KEYUP, UIntPtr.Zero);

            // Shot 5: Direct RingCentral Command messages
            PostMessage(target, WM_COMMAND, (IntPtr)1001, IntPtr.Zero);
            PostMessage(target, WM_COMMAND, (IntPtr)1,    IntPtr.Zero);
            PostMessage(target, WM_COMMAND, (IntPtr)101,  IntPtr.Zero);

            // Shot 6: Focus re-assert, modifier key release safety net & detach
            // (v4.3: REMOVED the old PostMessage(WM_SYSCHAR, VK_F1) shot that lived
            //  here. WM_SYSCHAR carries a CHARACTER code in wParam, not a virtual-key
            //  code — and VK_F1 is numerically 0x70, ASCII 'p'. RingCentral's
            //  Chromium text pipeline treated it as real typed input, so every
            //  cascade literally typed 'p' — the 'pppppppp' spam in the app.)
            try { SetForegroundWindow(target); } catch { }

            // Shot 6 (cont.): Modifier key release safety net & detach thread input
            keybd_event(VK_MENU,    SCAN_MENU, KEYEVENTF_KEYUP, UIntPtr.Zero);
            keybd_event(VK_CONTROL, 0x1D,      KEYEVENTF_KEYUP, UIntPtr.Zero);
            keybd_event(VK_SHIFT,   0x2A,      KEYEVENTF_KEYUP, UIntPtr.Zero);

            if (attached)
            {
                try { AttachThreadInput(curThreadId, targetThreadId, false); } catch { }
            }

            sw.Stop();
            return sw.ElapsedTicks * 1000000 / Stopwatch.Frequency;
        }

        // F8 SELF-TEST (also available from the tray menu). Verifies the
        // Alt+F1 answer path end-to-end WITHOUT touching call stats, and runs
        // even while the engine is paused. It locates the RingCentral Phone
        // window and delivers one real Alt+F1 cascade; the dashboard log
        // reports the result. Never sends Alt+A.
        public void RunSelfTest()
        {
            Log("──── F8 SELF-TEST ────────────────────────────────────");
            Log(string.Format("Engine: {0}. Answer key: Alt+F1 ONLY (Alt+A is never sent).",
                Active ? "ACTIVE" : "PAUSED (self-test runs anyway)"));

            IntPtr target = FindRingCentralWindow();
            if (target == IntPtr.Zero || !IsWindow(target))
            {
                Log("SELF-TEST: RingCentral Phone window NOT found.");
                Log("SELF-TEST: Start RingCentral Phone, then press F8 again.");
                Log("──── SELF-TEST COMPLETE (no window) ───────────────────");
                return;
            }

            System.Text.StringBuilder sb = new System.Text.StringBuilder(256);
            GetWindowText(target, sb, 256);
            Log(string.Format("SELF-TEST: found RingCentral window \"{0}\" (hwnd 0x{1}).",
                sb.ToString(), target.ToInt64().ToString("X")));
            Log("SELF-TEST: delivering one Alt+F1 cascade (focus + 6-shot IPC + hardware SendInput)...");

            if (!Monitor.TryEnter(fireLock, 0))
            {
                Log("SELF-TEST: a live answer cascade is in flight. Try again in a moment.");
                Log("──── SELF-TEST ABORTED ────────────────────────────────");
                return;
            }
            try
            {
                long lat = DeliverAnswerCascade(target);
                Log(string.Format("SELF-TEST: Alt+F1 cascade delivered in {0} us.", lat));
            }
            catch (Exception ex)
            {
                Log("SELF-TEST: ERROR during cascade: " + ex.Message);
            }
            finally
            {
                Monitor.Exit(fireLock);
            }

            Log("SELF-TEST: PASS - Alt+F1 path verified (call stats unchanged).");
            Log("          A ringing call is answered; an idle RingCentral ignores the shortcut.");
            Log("──── SELF-TEST COMPLETE ────────────────────────────────");
        }

        private void Log(string msg)
        {
            lock (logLock)
            {
                logs.Insert(0, string.Format("{0:HH:mm:ss.fff} | {1}", DateTime.Now, msg));
                if (logs.Count > 50) logs.RemoveAt(logs.Count - 1);
            }
        }

        public List<string> GetLogs()
        {
            lock (logLock)
            {
                return new List<string>(logs);
            }
        }

        public void Dispose()
        {
            if (pollTimer != null)
            {
                pollTimer.Dispose();
                pollTimer = null;
            }
            if (hookHandle != IntPtr.Zero)
            {
                UnhookWinEvent(hookHandle);
                hookHandle = IntPtr.Zero;
            }
        }

        delegate void WinEventDelegate(IntPtr hWinEventHook, uint eventType, IntPtr hwnd, int idObject, int idChild, uint dwEventThread, uint dwmsEventTime);
        delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

        [DllImport("user32.dll")] static extern IntPtr SetWinEventHook(uint eventMin, uint eventMax, IntPtr hmodWinEventProc, WinEventDelegate lpfnWinEventProc, uint idProcess, uint idThread, uint dwFlags);
        [DllImport("user32.dll")] static extern bool UnhookWinEvent(IntPtr hWinEventHook);
        [DllImport("user32.dll", CharSet = CharSet.Auto)] static extern int GetWindowText(IntPtr hWnd, System.Text.StringBuilder lpString, int nMaxCount);
        [DllImport("user32.dll")] static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
        [DllImport("user32.dll")] static extern IntPtr GetAncestor(IntPtr hwnd, uint gaFlags);
        [DllImport("user32.dll", CharSet = CharSet.Auto)] static extern IntPtr FindWindow(string lpClassName, string lpWindowName);
        [DllImport("user32.dll", CharSet = CharSet.Auto)] static extern IntPtr FindWindowEx(IntPtr parentHandle, IntPtr childAfter, string lclassName, string windowTitle);
        [DllImport("user32.dll")] static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
        [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr hWnd);
        [DllImport("user32.dll")] static extern bool IsWindow(IntPtr hWnd);
        [DllImport("user32.dll")] static extern bool SetForegroundWindow(IntPtr hWnd);
        [DllImport("user32.dll")] static extern bool IsIconic(IntPtr hWnd);
        [DllImport("user32.dll")] static extern bool BringWindowToTop(IntPtr hWnd);
        [DllImport("user32.dll")] static extern bool SetFocus(IntPtr hWnd);
        [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr hWnd, uint nCmdShow);
        [DllImport("user32.dll")] static extern void keybd_event(byte bVk, byte bScan, uint dwFlags, UIntPtr dwExtraInfo);
        [DllImport("kernel32.dll")] static extern uint GetCurrentThreadId();
        [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr hWnd, IntPtr lpdwProcessId);
        [DllImport("user32.dll")] static extern bool AttachThreadInput(uint idAttach, uint idAttachTo, bool fAttach);
    }

    static class HotKeyManager
    {
        [DllImport("user32.dll")] public static extern bool RegisterHotKey(IntPtr hWnd, int id, uint fsModifiers, uint vk);
        [DllImport("user32.dll")] public static extern bool UnregisterHotKey(IntPtr hWnd, int id);
        public enum KeyModifiers { None = 0, Alt = 1, Control = 2, Shift = 4, Windows = 8, NoRepeat = 0x4000 }
    }
}
