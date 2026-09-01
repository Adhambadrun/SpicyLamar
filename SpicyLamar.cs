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

        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            // Max-performance profile: real-time process priority (fall back to
            // high if the OS denies it) and a 1 ms multimedia timer tick.
            if (AnswerEngine.TURBO_MODE)
            {
                try { Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.RealTime; }
                catch { try { Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.High; } catch { } }
                timeBeginPeriod(1);
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

        public TerminalForm(AnswerEngine engine)
        {
            this.engine = engine;
            this.Text = "Spicy Lamar v4.2";
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
            }
            base.WndProc(ref m);
        }

        private void TerminalForm_Paint(object sender, PaintEventArgs e)
        {
            Graphics g = e.Graphics;

            g.DrawString("🌶️ SPICY LAMAR v4.2", headerFont, chiliBrush, 20, 20);
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
            string footer =
                AnswerEngine.BOUNDED_MODE
                    ? (AnswerEngine.TURBO_MODE
                        ? "F9 DASHBOARD   F11 PAUSE/START   F12 EXIT   ALT+F1 TURBO ANSWER (MAX 3/CALL)"
                        : "F9 DASHBOARD   F11 PAUSE/START   F12 EXIT   ALT+F1 ANSWER (MAX 3/CALL)")
                    : (AnswerEngine.TURBO_MODE
                        ? "F9 DASHBOARD   F11 PAUSE/START   F12 EXIT   ALT+F1 TURBO ATTENTION (50MS)"
                        : "F9 DASHBOARD   F11 PAUSE/START   F12 EXIT   ALT+F1 ALWAYS-ON ATTENTION");
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

        // ── Answer engine rate control ──────────────────────────────────────
        // BOUNDED_MODE=false (default): ALWAYS-ON ATTENTION — while a
        // RingCentral Phone window exists, Spicy Lamar relentlessly focuses
        // it and fires the Alt+F1 cascade, throttled only by DEBOUNCE_TICKS.
        // The app never goes quiet; use F11 (pause/start) to silence it.
        // BOUNDED_MODE=true: at most MAX_ATTEMPTS_PER_EPISODE cascades per
        // ringing episode, spaced MIN_RETRY_TICKS apart, re-armed after
        // EPISODE_RESET_TICKS of quiet (buttons not clicked as infinite).
        public const bool BOUNDED_MODE = false;
        // Max-performance profile (default for a PC dedicated to this job).
        // Fan out to the classic 20 ms scan / 0.5 s heart-beat by setting
        // this to false and rebuilding.
        public const bool TURBO_MODE = true;
        private const long DEBOUNCE_TICKS         = TURBO_MODE ? (50 * 10000)    : (500 * 10000);   // 50 ms / 500 ms in 100-ns ticks
        private const long MIN_RETRY_TICKS        = TURBO_MODE ? (100 * 10000)   : (1500 * 10000);  // 100 ms / 1500 ms in 100-ns ticks
        private const long EPISODE_RESET_TICKS    = TURBO_MODE ? (2000 * 10000)  : (10000 * 10000); // 2000 ms / 10000 ms in 100-ns ticks
        private const int  MAX_ATTEMPTS_PER_EPISODE = 3;
        private int episodeAttempts = 0;
        private bool capLogged = false;
        private readonly object fireLock = new object();

        private readonly object logLock = new object();
        private readonly object statsLock = new object();
        // Plain field: IntPtr cannot be declared volatile (CS0677); pointer-sized
        // reads/writes are atomic on x86/x64.
        private IntPtr cachedTarget = IntPtr.Zero;

        // Virtual key codes and keybd_event flags
        private const byte VK_MENU = 0x12;
        private const byte VK_RETURN = 0x0D;
        private const byte VK_CONTROL = 0x11;
        private const byte VK_F1 = 0x70;
        private const byte SCAN_MENU = 0x38;
        private const byte SCAN_F1 = 0x3B;
        private const uint KEYEVENTF_KEYUP = 0x0002;
        private const uint WM_SYSKEYDOWN = 0x0104;
        private const uint WM_SYSKEYUP = 0x0105;
        private const uint WM_KEYDOWN = 0x0100;
        private const uint WM_KEYUP = 0x0101;
        private const uint WM_COMMAND = 0x0111;
        private const uint SW_RESTORE = 0x0001;
        private const uint SW_SHOW = 0x0004;

        // INPUT struct for SendInput (hardware-level key synthesis)
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
            // Listen for window foreground, create, show, and title changes
            // dwFlags: WINEVENT_OUTOFCONTEXT (0) | WINEVENT_SKIPOWNPROCESS (2)
            const uint WINEVENT_OUTOFCONTEXT = 0;
            const uint WINEVENT_SKIPOWNPROCESS = 0x0002;
            hookHandle = SetWinEventHook(0x0003, 0x800C, IntPtr.Zero, dele, 0, 0,
                WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
            Log("Spicy Lamar v4.2 online. Hotkeys: F9 dashboard, F11 pause/start, F12 exit.");
            Log("Engine initialized. Call-event sensors active.");

            // Cache-refresh poll: 1 ms in the max-performance profile
            // (20 ms in the classic profile). NEVER fires the cascade itself —
            // an idle window must not be re-answered forever.
            pollTimer = new System.Threading.Timer(PollCheck, null, TURBO_MODE ? 1 : 100, TURBO_MODE ? 1 : 20);
        }

        private void PollCheck(object state)
        {
            // ALWAYS-ON ATTENTION poll: hunt for the RingCentral Phone window
            // and attend to it — focus + Alt+F1 answer cascade (throttled by
            // the engine's debounce). The window-event hook only makes it
            // respond faster; it is NOT required for it to work.
            IntPtr found = FindRingCentralWindow();
            cachedTarget = found;
            if (found != IntPtr.Zero) TryFire(found);
        }

        // Same title match set as the C++ version.
        private static bool IsTargetTitle(string title)
        {
            return title.IndexOf("RingCentral", StringComparison.OrdinalIgnoreCase) >= 0
                || title.IndexOf("Ring Central", StringComparison.OrdinalIgnoreCase) >= 0
                || title.IndexOf("RingMe", StringComparison.OrdinalIgnoreCase) >= 0
                || title.IndexOf("Glip", StringComparison.OrdinalIgnoreCase) >= 0;
        }

        private IntPtr FindRingCentralWindow()
        {
            // Fast path: reuse a still-valid cached target so the 1 ms scan
            // doesn't walk every top-level window on every tick. The WinEvent
            // hook catches new windows and refreshes the cache when the cached
            // window disappears.
            if (cachedTarget != IntPtr.Zero && IsWindow(cachedTarget))
            {
                return cachedTarget;
            }

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
            return found;
        }

        // Only these events indicate fresh call activity (ringing / call window
        // shown / activation). Firing on every window event would machine-gun
        // the answer shortcut at an idle window — which must never happen.
        private static bool IsAnswerTriggerEvent(uint eventType)
        {
            return eventType == 0x0003   // EVENT_SYSTEM_FOREGROUND
                || eventType == 0x8002   // EVENT_OBJECT_SHOW (call popup)
                || eventType == 0x800C;  // EVENT_OBJECT_NAMECHANGE (call state)
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
                TryFire(root);
            }
        }

        public void TryFire(IntPtr target)
        {
            // One cascade at a time: the 1 ms poll timer and the WinEvent hook
            // can race on different threads.
            if (!Monitor.TryEnter(fireLock, 0)) return;
            try
            {
            if (!Active) return;

            // Fall back to the cached target window (parity with the C++ version)
            if (target == IntPtr.Zero) target = cachedTarget;
            if (target == IntPtr.Zero || !IsWindow(target)) return;

            // ── Rate control ─────────────────────────────────────────────────
            // ALWAYS-ON ATTENTION (default): keep focusing + answering the
            // target window for as long as it exists — never goes quiet —
            // throttled only by the debounce. BOUNDED_MODE instead bounds
            // the cascades per ringing episode.
            long now = DateTime.UtcNow.Ticks;
            lock (fireLock)
            {
                long last = Interlocked.Read(ref lastFireTick);
                if (BOUNDED_MODE)
                {
                    if (last == 0 || (now - last) > EPISODE_RESET_TICKS)
                    {
                        episodeAttempts = 0;   // new episode: re-arm
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
                        return; // too soon after previous attempt
                    }
                    episodeAttempts++;
                }
                else
                {
                    if (last != 0 && (now - last) < DEBOUNCE_TICKS)
                    {
                        return; // relentless, but throttled
                    }
                }
                Interlocked.Exchange(ref lastFireTick, now);
            }

            Stopwatch sw = Stopwatch.StartNew();

            IntPtr child = FindWindowEx(target, IntPtr.Zero, "Chrome_RenderWidgetHostHWND", null);
            if (child == IntPtr.Zero)
            {
                IntPtr intermediate = FindWindowEx(target, IntPtr.Zero, "Intermediate D3D Window", null);
                if (intermediate != IntPtr.Zero)
                    child = FindWindowEx(intermediate, IntPtr.Zero, "Chrome_RenderWidgetHostHWND", null);
            }

            // ── Window activation FIRST — before any key messages ─────────────
            // PostMessage/PostMessage only reach a window that already has
            // foreground focus. The C++ version activates the window first
            // (restore + bring to top + foreground + child focus), then fires
            // the cascade. Match that ordering here so keys actually land.
            if (IsIconic(target)) ShowWindow(target, SW_RESTORE);
            BringWindowToTop(target);
            try { SetForegroundWindow(target); } catch { }
            if (child != IntPtr.Zero && IsWindow(child)) try { SetFocus(child); } catch { }

            // Shot 1: Target window Alt+F1 Down/Up
            PostMessage(target, WM_SYSKEYDOWN, (IntPtr)VK_MENU, (IntPtr)0x20380001);
            PostMessage(target, WM_SYSKEYDOWN, (IntPtr)VK_F1,   (IntPtr)0x203B0001);
            PostMessage(target, WM_SYSKEYUP,   (IntPtr)VK_F1,   (IntPtr)0xE03B0001);
            PostMessage(target, WM_KEYUP,      (IntPtr)VK_MENU, (IntPtr)0xE0380001);

            // Shot 2: Chrome Render Child Alt+F1 Down/Up
            if (child != IntPtr.Zero)
            {
                PostMessage(child, WM_SYSKEYDOWN, (IntPtr)VK_MENU, (IntPtr)0x20380001);
                PostMessage(child, WM_SYSKEYDOWN, (IntPtr)VK_F1,   (IntPtr)0x203B0001);
                PostMessage(child, WM_SYSKEYUP,   (IntPtr)VK_F1,   (IntPtr)0xE03B0001);
                PostMessage(child, WM_KEYUP,      (IntPtr)VK_MENU, (IntPtr)0xE0380001);
            }

            // Shot 3: Post Enter to root
            PostMessage(target, WM_KEYDOWN, (IntPtr)VK_RETURN, (IntPtr)0x001C0001);
            PostMessage(target, WM_KEYUP,   (IntPtr)VK_RETURN, (IntPtr)0xC01C0001);

            // Shot 4: hardware-level keyboard input via SendInput (most reliable)
            INPUT[] inputs = new INPUT[4];
            inputs[0] = new INPUT { type = INPUT_KEYBOARD, U = new INPUTUNION { ki = new KEYBDINPUT { wVk = VK_MENU } } };
            inputs[1] = new INPUT { type = INPUT_KEYBOARD, U = new INPUTUNION { ki = new KEYBDINPUT { wVk = VK_F1 } } };
            inputs[2] = new INPUT { type = INPUT_KEYBOARD, U = new INPUTUNION { ki = new KEYBDINPUT { wVk = VK_F1, dwFlags = KEYEVENTF_KEYUP } } };
            inputs[3] = new INPUT { type = INPUT_KEYBOARD, U = new INPUTUNION { ki = new KEYBDINPUT { wVk = VK_MENU, dwFlags = KEYEVENTF_KEYUP } } };
            SendInput(4, inputs, Marshal.SizeOf(typeof(INPUT)));

            // keybd_event legacy fallback (matches C++ Shot 4)
            keybd_event(VK_MENU, SCAN_MENU, 0, UIntPtr.Zero);
            keybd_event(VK_F1,   SCAN_F1,   0, UIntPtr.Zero);
            keybd_event(VK_F1,   SCAN_F1,   KEYEVENTF_KEYUP, UIntPtr.Zero);
            keybd_event(VK_MENU, SCAN_MENU, KEYEVENTF_KEYUP, UIntPtr.Zero);

            // Shot 5: Direct WM_COMMAND
            PostMessage(target, WM_COMMAND, (IntPtr)1001, IntPtr.Zero);

            // Shot 6: Foreground focus activation (re-assert after the cascade)
            try { SetForegroundWindow(target); } catch { }

            // Shot 7: Modifier key release safety net
            keybd_event(VK_MENU,    SCAN_MENU, KEYEVENTF_KEYUP, UIntPtr.Zero);
            keybd_event(VK_CONTROL, 0x1D,      KEYEVENTF_KEYUP, UIntPtr.Zero);

            sw.Stop();
            long lat = sw.ElapsedTicks * 1000000 / Stopwatch.Frequency;
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
            Log(string.Format("ANSWERED via 7-Shot Cascade in {0} us", lat));
            }
            finally { Monitor.Exit(fireLock); }
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
    }

    static class HotKeyManager
    {
        [DllImport("user32.dll")] public static extern bool RegisterHotKey(IntPtr hWnd, int id, uint fsModifiers, uint vk);
        [DllImport("user32.dll")] public static extern bool UnregisterHotKey(IntPtr hWnd, int id);
        public enum KeyModifiers { None = 0, Alt = 1, Control = 2, Shift = 4, Windows = 8, NoRepeat = 0x4000 }
    }
}
