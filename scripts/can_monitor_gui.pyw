"""
Taktflow CAN Bus Monitor — standalone desktop GUI.
Double-click to launch. Reads from Waveshare USB-CAN-A on COM13.
"""

import tkinter as tk
from tkinter import ttk
import serial
import struct
import time
import threading

# ── Config ──────────────────────────────────────────────────────────────
SERIAL_PORT = "COM13"
SERIAL_BAUD = 2000000
REFRESH_MS = 200  # GUI refresh rate

CAN_ID_NAMES = {
    0x001: "CVC E-Stop",       0x010: "CVC Heartbeat",
    0x100: "CVC Vehicle State", 0x101: "CVC Torque Cmd",
    0x102: "CVC Steering Cmd", 0x103: "CVC Brake Cmd",
    0x350: "CVC Body Ctrl",    0x500: "DTC (shared)",
    0x7E8: "CVC UDS Response",
    0x011: "FZC Heartbeat",    0x200: "FZC Steering Status",
    0x201: "FZC Brake Status", 0x210: "FZC Brake Fault",
    0x211: "FZC Motor Cutoff", 0x220: "FZC Lidar",
    0x012: "RZC Heartbeat",    0x300: "RZC Motor Status",
    0x301: "RZC Motor Current", 0x302: "RZC Motor Temp",
    0x303: "RZC Battery",
    0x7DF: "UDS Broadcast",    0x7E0: "UDS Request",
}

VSM_NAMES = {0: "INIT", 1: "INIT", 2: "RUN", 3: "SAFE_STOP", 4: "FAULT"}

# ECU color tags
ECU_COLORS = {
    "CVC": "#e0e7ff",  # light indigo
    "FZC": "#fce7f3",  # light pink
    "RZC": "#d1fae5",  # light green
    "???": "#f3f4f6",  # light gray
}


def ecu_of(can_id):
    if can_id <= 0x0FF:
        return "CVC"
    elif can_id <= 0x1FF:
        return "FZC"
    elif can_id <= 0x2FF:
        return "RZC"
    return "???"


class CanMonitor:
    def __init__(self):
        self.lock = threading.Lock()
        self.latest = {}   # can_id -> {data, timestamp, count}
        self.total = 0
        self.t0 = time.time()
        self.running = False
        self.ser = None
        self.error = None

    def connect(self, port, baud):
        try:
            self.ser = serial.Serial(port, baud, timeout=0.05)
            self.running = True
            self.t0 = time.time()
            self.error = None
            return True
        except serial.SerialException as e:
            self.error = str(e)
            return False

    def disconnect(self):
        self.running = False
        if self.ser:
            try:
                self.ser.close()
            except Exception:
                pass
            self.ser = None

    def read_loop(self):
        buf = bytearray()
        while self.running and self.ser:
            try:
                chunk = self.ser.read(max(1, self.ser.in_waiting))
            except Exception:
                break
            if chunk:
                buf.extend(chunk)

            while len(buf) >= 20:
                idx = -1
                for i in range(len(buf) - 1):
                    if buf[i] == 0xAA and buf[i + 1] == 0x55:
                        idx = i
                        break
                if idx < 0:
                    buf = buf[-1:]
                    break
                if idx > 0:
                    buf = buf[idx:]
                if len(buf) < 20:
                    break

                fb = bytes(buf[:20])
                buf = buf[20:]

                frame_type = fb[3]
                can_id = struct.unpack_from("<I", fb, 5)[0]
                if frame_type == 0x01:
                    can_id &= 0x7FF
                else:
                    can_id &= 0x1FFFFFFF
                dlc = min(fb[9], 8)
                data = fb[10:10 + dlc]
                elapsed = time.time() - self.t0

                with self.lock:
                    self.total += 1
                    if can_id not in self.latest:
                        self.latest[can_id] = {"count": 0}
                    self.latest[can_id]["data"] = data
                    self.latest[can_id]["timestamp"] = elapsed
                    self.latest[can_id]["count"] += 1

    def snapshot(self):
        with self.lock:
            elapsed = time.time() - self.t0
            return dict(self.latest), self.total, elapsed


class App:
    def __init__(self, root):
        self.root = root
        self.root.title("Taktflow CAN Bus Monitor")
        self.root.geometry("920x620")
        self.root.configure(bg="#1e1b2e")

        self.monitor = CanMonitor()
        self.thread = None

        self._build_ui()
        self._try_connect()

    def _build_ui(self):
        # ── Title bar ───────────────────────────────────────────────
        top = tk.Frame(self.root, bg="#2d2640", pady=8)
        top.pack(fill="x")

        self.title_label = tk.Label(
            top, text="TAKTFLOW CAN BUS MONITOR",
            font=("Consolas", 14, "bold"), fg="#c4b5fd", bg="#2d2640"
        )
        self.title_label.pack(side="left", padx=16)

        self.status_label = tk.Label(
            top, text="Connecting...",
            font=("Consolas", 10), fg="#9ca3af", bg="#2d2640"
        )
        self.status_label.pack(side="right", padx=16)

        # ── CAN frame table ────────────────────────────────────────
        table_frame = tk.Frame(self.root, bg="#1e1b2e")
        table_frame.pack(fill="both", expand=True, padx=8, pady=(4, 0))

        style = ttk.Style()
        style.theme_use("clam")
        style.configure("Can.Treeview",
                        background="#1e1b2e", foreground="#e5e7eb",
                        fieldbackground="#1e1b2e", borderwidth=0,
                        font=("Consolas", 10), rowheight=24)
        style.configure("Can.Treeview.Heading",
                        background="#2d2640", foreground="#c4b5fd",
                        font=("Consolas", 10, "bold"), borderwidth=0)
        style.map("Can.Treeview",
                  background=[("selected", "#4c1d95")],
                  foreground=[("selected", "#ffffff")])

        cols = ("id", "name", "data", "count", "rate", "age")
        self.tree = ttk.Treeview(
            table_frame, columns=cols, show="headings",
            style="Can.Treeview"
        )
        self.tree.heading("id", text="CAN ID")
        self.tree.heading("name", text="Message")
        self.tree.heading("data", text="Data")
        self.tree.heading("count", text="Count")
        self.tree.heading("rate", text="Rate")
        self.tree.heading("age", text="Age")

        self.tree.column("id", width=70, anchor="center")
        self.tree.column("name", width=200, anchor="w")
        self.tree.column("data", width=260, anchor="w")
        self.tree.column("count", width=80, anchor="e")
        self.tree.column("rate", width=80, anchor="e")
        self.tree.column("age", width=60, anchor="e")

        scrollbar = ttk.Scrollbar(table_frame, orient="vertical",
                                  command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")

        # Row color tags
        self.tree.tag_configure("cvc", background="#1a1840")
        self.tree.tag_configure("fzc", background="#2a1530")
        self.tree.tag_configure("rzc", background="#0f2518")
        self.tree.tag_configure("unknown", background="#1e1b2e")

        # ── ECU health bar ──────────────────────────────────────────
        health_frame = tk.Frame(self.root, bg="#2d2640", pady=6)
        health_frame.pack(fill="x", padx=8, pady=(4, 8))

        self.ecu_labels = {}
        for ecu in ("CVC", "FZC", "RZC"):
            lbl = tk.Label(
                health_frame, text=f"  {ecu}: --  ",
                font=("Consolas", 11, "bold"), fg="#9ca3af",
                bg="#2d2640", padx=12
            )
            lbl.pack(side="left", padx=8)
            self.ecu_labels[ecu] = lbl

        # ── Reconnect button (hidden until needed) ──────────────────
        self.reconnect_btn = tk.Button(
            health_frame, text="Reconnect", font=("Consolas", 10),
            fg="#e5e7eb", bg="#4c1d95", activebackground="#6d28d9",
            command=self._try_connect, bd=0, padx=12, pady=2
        )
        self.reconnect_btn.pack(side="right", padx=16)

    def _try_connect(self):
        self.monitor.disconnect()
        if self.thread and self.thread.is_alive():
            self.thread.join(timeout=1)

        self.monitor = CanMonitor()
        if self.monitor.connect(SERIAL_PORT, SERIAL_BAUD):
            self.status_label.config(
                text=f"{SERIAL_PORT} connected", fg="#34d399"
            )
            self.thread = threading.Thread(
                target=self.monitor.read_loop, daemon=True
            )
            self.thread.start()
            self._schedule_refresh()
        else:
            self.status_label.config(
                text=f"Cannot open {SERIAL_PORT}", fg="#f87171"
            )

    def _schedule_refresh(self):
        if self.monitor.running:
            self._refresh()
            self.root.after(REFRESH_MS, self._schedule_refresh)

    def _refresh(self):
        snapshot, total, elapsed = self.monitor.snapshot()
        fps = total / max(elapsed, 0.001)

        self.status_label.config(
            text=f"{elapsed:.0f}s | {total:,} frames | {fps:.0f} fps",
            fg="#34d399"
        )

        # Update tree
        existing_ids = set()
        for item in self.tree.get_children():
            existing_ids.add(self.tree.item(item, "values")[0])

        for cid in sorted(snapshot.keys()):
            entry = snapshot[cid]
            name = CAN_ID_NAMES.get(cid, "???")
            data_hex = " ".join(f"{b:02X}" for b in entry["data"])
            count = entry["count"]
            rate = f"{count / max(elapsed, 0.001):.1f}/s"
            age = elapsed - entry["timestamp"]
            age_str = f"{age:.1f}s" if age < 10 else f"{age:.0f}s"
            id_str = f"0x{cid:03X}"

            ecu = ecu_of(cid)
            tag = ecu.lower() if ecu != "???" else "unknown"

            values = (id_str, name, data_hex, f"{count:,}", rate, age_str)

            # Update existing or insert new
            found = False
            for item in self.tree.get_children():
                if self.tree.item(item, "values")[0] == id_str:
                    self.tree.item(item, values=values, tags=(tag,))
                    found = True
                    break
            if not found:
                self.tree.insert("", "end", values=values, tags=(tag,))

        # ECU health
        for ecu, hb_id in [("CVC", 0x010), ("FZC", 0x011), ("RZC", 0x012)]:
            lbl = self.ecu_labels[ecu]
            if hb_id in snapshot:
                age = elapsed - snapshot[hb_id]["timestamp"]
                d = snapshot[hb_id]["data"]
                alive = ((d[0] >> 4) & 0x0F) if len(d) > 0 else 0
                vsm = (d[3] & 0x0F) if len(d) > 3 else 0
                vsm_str = VSM_NAMES.get(vsm, f"0x{vsm:02X}")

                if age < 0.3:
                    status, color = "OK", "#34d399"
                elif age < 1.0:
                    status, color = "STALE", "#fbbf24"
                else:
                    status, color = "DEAD", "#f87171"

                lbl.config(
                    text=f"  {ecu}: {status}  vsm={vsm_str}  alive={alive}  ",
                    fg=color
                )
            else:
                lbl.config(text=f"  {ecu}: --  ", fg="#9ca3af")

    def on_close(self):
        self.monitor.disconnect()
        self.root.destroy()


def main():
    root = tk.Tk()
    app = App(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()


if __name__ == "__main__":
    main()
