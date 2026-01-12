import tkinter as tk
from tkinter import messagebox
import serial

PORT = "COM6"      # <-- change this
BAUD = 115200

class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Arduino DAC Control")

        try:
            self.ser = serial.Serial(PORT, BAUD, timeout=0.2)
        except Exception as e:
            messagebox.showerror("Serial error", f"Cannot open {PORT}\n\n{e}")
            self.destroy()
            return

        tk.Label(self, text="Channel", width=10, anchor="w").grid(row=0, column=0, padx=6, pady=4)
        tk.Label(self, text="Voltage (V)", width=15, anchor="w").grid(row=0, column=1, padx=6, pady=4)
        tk.Label(self, text="Confirm", width=10, anchor="w").grid(row=0, column=2, padx=6, pady=4)

        self.entries = []
        for ch in range(16):
            tk.Label(self, text=str(ch), width=10, anchor="w").grid(row=ch+1, column=0, padx=6, pady=2)

            e = tk.Entry(self, width=15)
            e.insert(0, "0.0000")
            e.grid(row=ch+1, column=1, padx=6, pady=2)
            self.entries.append(e)

            tk.Button(self, text="Confirm", command=lambda c=ch: self.send(c)).grid(
                row=ch+1, column=2, padx=6, pady=2
            )

        self.status = tk.StringVar(value=f"Connected to {PORT} @ {BAUD}")
        tk.Label(self, textvariable=self.status, anchor="w").grid(
            row=17, column=0, columnspan=3, sticky="we", padx=6, pady=8
        )

        self.protocol("WM_DELETE_WINDOW", self.on_close)

    def send(self, ch: int):
        txt = self.entries[ch].get().strip()
        try:
            v = float(txt)
        except ValueError:
            messagebox.showerror("Input error", f"Channel {ch}: not a number")
            return

        # If your DAC range is -5..+5, keep this check
        if v < -5.0 or v > 5.0:
            messagebox.showerror("Range error", f"Channel {ch}: voltage must be between -5.0 and +5.0 V")
            return

        cmd = f"SET {ch} {v:.4f}\n"
        try:
            self.ser.write(cmd.encode())
            self.status.set(f"Sent: {cmd.strip()}")
        except Exception as e:
            messagebox.showerror("Serial write error", str(e))

    def on_close(self):
        try:
            if hasattr(self, "ser") and self.ser:
                self.ser.close()
        except Exception:
            pass
        self.destroy()

if __name__ == "__main__":
    App().mainloop()
