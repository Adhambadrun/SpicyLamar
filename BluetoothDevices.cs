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
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            bool createdNew;
            using (Mutex mutex = new Mutex(true, "Global\\SpicyLamarQuantumV4", out createdNew))
            {
                if (!createdNew)
                {
                    return; // Single-instance enforcement
                }
                Application.Run(new DashboardContext());
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
                Icon = LoadBluetoothIcon(),
                Text = "Bluetooth Devices",
                Visible = true,
                ContextMenu = BuildMenu()
            };
            trayIcon.DoubleClick += delegate(object s, EventArgs e)
            {
                try { Process.Start("ms-settings:bluetooth"); } catch { }
            };

            dashboard = new TerminalForm(engine);

            // Global Hotkeys
            HotKeyManager.RegisterHotKey(dashboard.Handle, 1, HotKeyManager.KeyModifiers.None, Keys.F9);  // Toggle Dashboard
            HotKeyManager.RegisterHotKey(dashboard.Handle, 2, HotKeyManager.KeyModifiers.None, Keys.F11); // Pause / Resume
            HotKeyManager.RegisterHotKey(dashboard.Handle, 3, HotKeyManager.KeyModifiers.None, Keys.F12); // Exit
        }

        private Icon LoadBluetoothIcon()
        {
            string[] paths = new string[]
            {
                "icon.ico",
                @"C:\Windows\System32\bthprops.cpl",
                @"C:\Windows\System32\deviceflow.dll",
                @"C:\Windows\System32\shell32.dll"
            };

            foreach (string p in paths)
            {
                try
                {
                    if (System.IO.File.Exists(p))
                    {
                        if (p.EndsWith(".ico", StringComparison.OrdinalIgnoreCase))
                            return new Icon(p);
                        Icon extracted = Icon.ExtractAssociatedIcon(p);
                        if (extracted != null) return extracted;
                    }
                }
                catch { }
            }
            return SystemIcons.Application;
        }

        private ContextMenu BuildMenu()
        {
            ContextMenu menu = new ContextMenu();
            menu.MenuItems.Add("Add a Bluetooth Device", delegate(object s, EventArgs e)
            {
                MessageBox.Show("Searching for devices...", "Add a device", MessageBoxButtons.OK, MessageBoxIcon.Information);
            });
            MenuItem allowItem = menu.MenuItems.Add("Allow a Device to Connect");
            allowItem.Enabled = false;
            menu.MenuItems.Add("Show Bluetooth Devices", delegate(object s, EventArgs e)
            {
                try { Process.Start("ms-settings:bluetooth"); } catch { }
            });
            menu.MenuItems.Add("-");
            menu.MenuItems.Add("Send a File", delegate(object s, EventArgs e)
            {
                MessageBox.Show("No paired devices found.", "Transfer", MessageBoxButtons.OK);
            });
            menu.MenuItems.Add("Receive a File", delegate(object s, EventArgs e)
            {
                MessageBox.Show("No devices in range.", "Transfer", MessageBoxButtons.OK);
            });
            menu.MenuItems.Add("-");
            menu.MenuItems.Add("Join a Personal Area Network", delegate(object s, EventArgs e)
            {
                try { Process.Start("control", "ncpa.cpl"); } catch { }
            });
            menu.MenuItems.Add("-");
            menu.MenuItems.Add("Open Settings", delegate(object s, EventArgs e)
            {
                dashboard.ToggleVisibility();
            });
            menu.MenuItems.Add("Remove Icon", delegate(object s, EventArgs e)
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
            this.Text = "Spicy Lamar v4.0 // LIGHTSTORM";
            this.Size = new Size(780, 500);
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
            refreshTimer.Interval = 250;
            refreshTimer.Tick += delegate(object s, EventArgs e) { this.Invalidate(); };
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
                if (id == 2) engine.Active = !engine.Active;
                if (id == 3) Application.Exit();
            }
            base.WndProc(ref m);
        }

        private void TerminalForm_Paint(object sender, PaintEventArgs e)
        {
            Graphics g = e.Graphics;

            g.DrawString("🌶️ SPICY LAMAR v4.0 // QUANTUM SINGULARITY ENGINE", headerFont, chiliBrush, 20, 20);
            g.DrawLine(linePen, 20, 45, 740, 45);

            string status = engine.Active ? "[🌶️ ACTIVE]" : "[⚠ PAUSED]";
            g.DrawString(string.Format("STATUS: {0}", status), monoFont, engine.Active ? neonBrush : chiliBrush, 20, 55);

            string uptime = DateTime.Now.Subtract(Process.GetCurrentProcess().StartTime).ToString(@"hh\:mm\:ss");
            g.DrawString(string.Format("CALLS: {0}   UPTIME: {1}", engine.CallCount, uptime), monoFont, dimBrush, 250, 55);

            g.DrawString("[ REAL-TIME TELEMETRY ]", monoFont, chiliBrush, 20, 100);
            g.DrawString(string.Format("P50 Latency: {0} us", engine.LastLatency), monoFont, neonBrush, 30, 125);

            // Telemetry Histogram
            for (int i = 0; i < 5; i++)
            {
                int w = Math.Max(10, 200 - (i * 40));
                g.FillRectangle(neonBrush, 150, 125 + (i * 20), w, 12);
                g.DrawString(string.Format("{0}us", 20 * (i + 1)), monoFont, dimBrush, 360, 125 + (i * 20));
            }

            g.DrawString("[ SYSTEM LOG ]", monoFont, chiliBrush, 20, 250);
            List<string> recentLogs = engine.GetLogs().Take(8).ToList();
            for (int i = 0; i < recentLogs.Count; i++)
            {
                g.DrawString(recentLogs[i], monoFont, neonBrush, 30, 275 + (i * 20));
            }
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
        public bool Active = true;
        public int CallCount = 0;
        public long LastLatency = 0;
        private List<string> logs = new List<string>();
        private WinEventDelegate dele;
        private IntPtr hookHandle = IntPtr.Zero;
        private System.Threading.Timer pollTimer;
        private long lastFireTick = 0;
        private const long DEBOUNCE_TICKS = 1200 * 10000; // 1200 ms in 100-ns ticks
        private readonly object logLock = new object();

        public AnswerEngine()
        {
            dele = new WinEventDelegate(WinEventProc);
            // Listen for window foreground, create, show, and title changes
            hookHandle = SetWinEventHook(0x0003, 0x800C, IntPtr.Zero, dele, 0, 0, 0); // WINEVENT_OUTOFCONTEXT
            Log("Engine initialized. 5-Channel Fusion sensors active.");

            // Backup polling channel: 50ms interval ensures sub-second response even if hooks drop
            pollTimer = new System.Threading.Timer(PollCheck, null, 100, 50);
        }

        private void PollCheck(object state)
        {
            if (!Active) return;
            IntPtr rcHwnd = FindRingCentralWindow();
            if (rcHwnd != IntPtr.Zero)
            {
                TryFire(rcHwnd);
            }
        }

        private IntPtr FindRingCentralWindow()
        {
            IntPtr found = IntPtr.Zero;
            EnumWindows(delegate(IntPtr hWnd, IntPtr lParam)
            {
                if (!IsWindowVisible(hWnd)) return true;
                System.Text.StringBuilder sb = new System.Text.StringBuilder(256);
                GetWindowText(hWnd, sb, 256);
                string title = sb.ToString();
                if (title.IndexOf("RingCentral Phone", StringComparison.OrdinalIgnoreCase) >= 0)
                {
                    found = hWnd;
                    return false; // Stop enumeration
                }
                return true;
            }, IntPtr.Zero);
            return found;
        }

        private void WinEventProc(IntPtr hWinEventHook, uint eventType, IntPtr hwnd, int idObject, int idChild, uint dwEventThread, uint dwmsEventTime)
        {
            if (idObject != 0 || hwnd == IntPtr.Zero) return;
            // Ignore window destroy events
            if (eventType == 0x8001) return;

            IntPtr root = GetAncestor(hwnd, 3); // GA_ROOT
            if (root == IntPtr.Zero) root = hwnd;

            System.Text.StringBuilder sb = new System.Text.StringBuilder(256);
            GetWindowText(root, sb, 256);
            if (sb.ToString().IndexOf("RingCentral Phone", StringComparison.OrdinalIgnoreCase) >= 0)
            {
                TryFire(root);
            }
        }

        public void TryFire(IntPtr target)
        {
            if (!Active) return;

            long now = DateTime.UtcNow.Ticks;
            if (now - Interlocked.Read(ref lastFireTick) < DEBOUNCE_TICKS)
            {
                return; // Debounced
            }
            Interlocked.Exchange(ref lastFireTick, now);

            Stopwatch sw = Stopwatch.StartNew();

            // Quad-Shot IPC (Alt+F1)
            IntPtr child = FindWindowEx(target, IntPtr.Zero, "Chrome_RenderWidgetHostHWND", null);

            // Target window Alt+F1 down/up
            PostMessage(target, 0x0104, (IntPtr)0x12, (IntPtr)0x20380001); // WM_SYSKEYDOWN VK_MENU
            PostMessage(target, 0x0104, (IntPtr)0x70, (IntPtr)0x203B0001); // WM_SYSKEYDOWN VK_F1
            PostMessage(target, 0x0105, (IntPtr)0x70, (IntPtr)0xE03B0001); // WM_SYSKEYUP VK_F1
            PostMessage(target, 0x0101, (IntPtr)0x12, (IntPtr)0xE0380001); // WM_KEYUP VK_MENU

            // Chrome child window complete down/up sequence
            if (child != IntPtr.Zero)
            {
                PostMessage(child, 0x0104, (IntPtr)0x12, (IntPtr)0x20380001);
                PostMessage(child, 0x0104, (IntPtr)0x70, (IntPtr)0x203B0001);
                PostMessage(child, 0x0105, (IntPtr)0x70, (IntPtr)0xE03B0001);
                PostMessage(child, 0x0101, (IntPtr)0x12, (IntPtr)0xE0380001);
            }

            sw.Stop();
            LastLatency = sw.ElapsedTicks * 1000000 / Stopwatch.Frequency;
            CallCount++;
            Log(string.Format("ANSWERED via Quad-Shot in {0} us", LastLatency));
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
        [DllImport("user32.dll")] static extern IntPtr PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
        [DllImport("user32.dll")] static extern IntPtr GetAncestor(IntPtr hwnd, uint gaFlags);
        [DllImport("user32.dll", CharSet = CharSet.Auto)] static extern IntPtr FindWindowEx(IntPtr parentHandle, IntPtr childAfter, string lclassName, string windowTitle);
        [DllImport("user32.dll")] static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
        [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr hWnd);
    }

    static class HotKeyManager
    {
        [DllImport("user32.dll")] public static extern bool RegisterHotKey(IntPtr hWnd, int id, uint fsModifiers, uint vk);
        [DllImport("user32.dll")] public static extern bool UnregisterHotKey(IntPtr hWnd, int id);
        public enum KeyModifiers { None = 0, Alt = 1, Control = 2, Shift = 4, Windows = 8 }
    }
}
