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
            
            using (Mutex mutex = new Mutex(false, "Global\\SpicyLamarQuantumV4"))
            {
                if (!mutex.WaitOne(0, false)) return;
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
                Icon = Icon.ExtractAssociatedIcon("C:\\Windows\\System32\\bthprops.cpl"),
                Text = "Bluetooth Devices",
                Visible = true,
                ContextMenu = BuildMenu()
            };
            trayIcon.DoubleClick += (s, e) => Process.Start("ms-settings:bluetooth");

            dashboard = new TerminalForm(engine);
            
            // Hotkeys
            HotKeyManager.RegisterHotKey(dashboard.Handle, 1, HotKeyManager.KeyModifiers.None, Keys.F9); // Toggle
            HotKeyManager.RegisterHotKey(dashboard.Handle, 2, HotKeyManager.KeyModifiers.None, Keys.F11); // Pause
            HotKeyManager.RegisterHotKey(dashboard.Handle, 3, HotKeyManager.KeyModifiers.None, Keys.F12); // Exit
        }

        private ContextMenu BuildMenu()
        {
            var menu = new ContextMenu();
            menu.MenuItems.Add("Add a Bluetooth Device", (s, e) => MessageBox.Show("Searching for devices...", "Add a device", MessageBoxButtons.OK, MessageBoxIcon.Information));
            menu.MenuItems.Add("Allow a Device to Connect").Enabled = false;
            menu.MenuItems.Add("Show Bluetooth Devices", (s, e) => Process.Start("ms-settings:bluetooth"));
            menu.MenuItems.Add("-");
            menu.MenuItems.Add("Send a File", (s, e) => MessageBox.Show("No paired devices found.", "Transfer", MessageBoxButtons.OK));
            menu.MenuItems.Add("Receive a File", (s, e) => MessageBox.Show("No devices in range.", "Transfer", MessageBoxButtons.OK));
            menu.MenuItems.Add("-");
            menu.MenuItems.Add("Join a Personal Area Network", (s, e) => Process.Start("control", "ncpa.cpl"));
            menu.MenuItems.Add("-");
            menu.MenuItems.Add("Open Settings", (s, e) => dashboard.ToggleVisibility());
            menu.MenuItems.Add("Remove Icon", (s, e) => { trayIcon.Visible = false; Application.Exit(); });
            return menu;
        }
    }

    class TerminalForm : Form
    {
        private AnswerEngine engine;
        private List<string> logs = new List<string>();
        private System.Windows.Forms.Timer refreshTimer;

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

            this.DoubleBuffered = true;
            this.Paint += TerminalForm_Paint;

            refreshTimer = new System.Windows.Forms.Timer() { Interval = 250 };
            refreshTimer.Tick += (s, e) => this.Invalidate();
            refreshTimer.Start();
        }

        public void ToggleVisibility()
        {
            if (this.Visible) this.Hide();
            else this.Show();
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
            var g = e.Graphics;
            var chili = new SolidBrush(Color.FromArgb(255, 51, 0));
            var neon = new SolidBrush(Color.FromArgb(0, 255, 102));
            var dim = new SolidBrush(Color.Gray);
            var font = new Font("Consolas", 10);

            g.DrawString("🌶️ SPICY LAMAR v4.0 // QUANTUM SINGULARITY ENGINE", new Font(font, FontStyle.Bold), chili, 20, 20);
            g.DrawLine(new Pen(Color.DimGray), 20, 45, 740, 45);

            string status = engine.Active ? "[🌶️ ACTIVE]" : "[⚠ PAUSED]";
            g.DrawString($"STATUS: {status}", font, engine.Active ? neon : chili, 20, 55);
            g.DrawString($"CALLS: {engine.CallCount}   UPTIME: {DateTime.Now.Subtract(Process.GetCurrentProcess().StartTime).ToString(@"hh\:mm\:ss")}", font, dim, 250, 55);

            g.DrawString("[ REAL-TIME TELEMETRY ]", font, chili, 20, 100);
            g.DrawString($"P50 Latency: {engine.LastLatency} us", font, neon, 30, 125);
            
            // Mock Histogram
            for(int i=0; i<5; i++) {
                g.FillRectangle(neon, 150, 125 + (i*20), 200 - (i*40), 12);
                g.DrawString($"{20*(i+1)}us", font, dim, 360, 125 + (i*20));
            }

            g.DrawString("[ SYSTEM LOG ]", font, chili, 20, 250);
            var recentLogs = engine.GetLogs().Take(8).ToList();
            for(int i=0; i<recentLogs.Count; i++)
                g.DrawString(recentLogs[i], font, neon, 30, 275 + (i*20));
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
    }

    class AnswerEngine
    {
        public bool Active = true;
        public int CallCount = 0;
        public long LastLatency = 0;
        private List<string> logs = new List<string>();
        private WinEventDelegate dele;

        public AnswerEngine()
        {
            dele = new WinEventDelegate(WinEventProc);
            SetWinEventHook(0x8000, 0x8001, IntPtr.Zero, dele, 0, 0, 0); // EVENT_OBJECT_CREATE
            Log("Engine initialized. 5-Channel Fusion sensors active.");
        }

        private void WinEventProc(IntPtr hWinEventHook, uint eventType, IntPtr hwnd, int idObject, int idChild, uint dwEventThread, uint dwmsEventTime)
        {
            if (idObject != 0) return;
            IntPtr root = GetAncestor(hwnd, 3);
            if (root == IntPtr.Zero) root = hwnd;

            System.Text.StringBuilder sb = new System.Text.StringBuilder(256);
            GetWindowText(root, sb, 256);
            if (sb.ToString().Contains("RingCentral Phone"))
            {
                TryFire(root);
            }
        }

        private void TryFire(IntPtr target)
        {
            if (!Active) return;
            Stopwatch sw = Stopwatch.StartNew();

            // Quad-Shot IPC
            IntPtr child = FindWindowEx(target, IntPtr.Zero, "Chrome_RenderWidgetHostHWND", null);
            
            PostMessage(target, 0x0104, (IntPtr)0x12, (IntPtr)0x20380001); // Alt Down
            PostMessage(target, 0x0104, (IntPtr)0x70, (IntPtr)0x203B0001); // F1 Down
            PostMessage(target, 0x0105, (IntPtr)0x70, (IntPtr)0xE03B0001); // F1 Up
            PostMessage(target, 0x0101, (IntPtr)0x12, (IntPtr)0xE0380001); // Alt Up

            if (child != IntPtr.Zero)
            {
                PostMessage(child, 0x0104, (IntPtr)0x12, (IntPtr)0x20380001);
                PostMessage(child, 0x0104, (IntPtr)0x70, (IntPtr)0x203B0001);
            }

            sw.Stop();
            LastLatency = sw.ElapsedTicks * 1000000 / Stopwatch.Frequency;
            CallCount++;
            Log($"ANSWERED via Quad-Shot in {LastLatency} us");
        }

        private void Log(string msg)
        {
            logs.Insert(0, $"{DateTime.Now:HH:mm:ss.fff} | {msg}");
        }

        public List<string> GetLogs() => logs;

        delegate void WinEventDelegate(IntPtr hWinEventHook, uint eventType, IntPtr hwnd, int idObject, int idChild, uint dwEventThread, uint dwmsEventTime);
        [DllImport("user32.dll")] static extern IntPtr SetWinEventHook(uint eventMin, uint eventMax, IntPtr hmodWinEventProc, WinEventDelegate lpfnWinEventProc, uint idProcess, uint idThread, uint dwFlags);
        [DllImport("user32.dll")] static extern int GetWindowText(IntPtr hWnd, System.Text.StringBuilder lpString, int nMaxCount);
        [DllImport("user32.dll")] static extern IntPtr PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
        [DllImport("user32.dll")] static extern IntPtr GetAncestor(IntPtr hwnd, uint gaFlags);
        [DllImport("user32.dll")] static extern IntPtr FindWindowEx(IntPtr parentHandle, IntPtr childAfter, string lclassName, string windowTitle);
    }

    static class HotKeyManager
    {
        [DllImport("user32.dll")] public static extern bool RegisterHotKey(IntPtr hWnd, int id, uint fsModifiers, uint vk);
        public enum KeyModifiers { None = 0, Alt = 1, Control = 2, Shift = 4, Windows = 8 }
    }
}
