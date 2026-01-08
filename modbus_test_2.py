import minimalmodbus
import time
import threading
import tkinter as tk
from tkinter import ttk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
from matplotlib.ticker import MaxNLocator

# --- Konfiguracja urządzenia ---
instrument = minimalmodbus.Instrument('COM5', 2, debug=False)
instrument.serial.baudrate = 115200
instrument.serial.bytesize = 8
instrument.serial.parity = 'N'
instrument.serial.stopbits = 1
instrument.serial.timeout = 1
instrument.mode = minimalmodbus.MODE_RTU


# --- Funkcje pomocnicze ---
def to_signed_16bit(val):
    return val - 0x10000 if val > 0x7FFF else val


def safe_read_registers(start, count, retries=5, delay=0.05):
    for _ in range(retries):
        try:
            return instrument.read_registers(start, count)
        except IOError:
            time.sleep(delay)
    raise IOError("Brak odpowiedzi z urządzenia.")


def safe_read_register(address, retries=5, delay=0.05):
    for _ in range(retries):
        try:
            return instrument.read_register(address)
        except IOError:
            time.sleep(delay)
    raise IOError("Brak odpowiedzi z urządzenia.")


class ModbusGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Modbus Temperature Monitor")
        self.running = False

        # Historia danych
        self.data_history = [[] for _ in range(10)]
        self.tmp_history = []

        # --- Bufory czasów ---
        self.time_block = []
        self.time_single = []
        self.time_cycle = []

        # --- GUI ---
        main_frame = ttk.Frame(root, padding=10)
        main_frame.pack(fill="both", expand=True)

        label_frame = ttk.Frame(main_frame)
        label_frame.pack(pady=5)

        self.labels = []
        layout = [(0,0),(1,0),(2,0),(3,0),(4,0),
                  (0,1),(1,1),(2,1),(3,1),(4,1)]

        for i, (r, c) in enumerate(layout):
            lbl = ttk.Label(label_frame, text=f"Sensor {i+1}: -- °C",
                            font=("Segoe UI", 12))
            lbl.grid(row=r, column=c, padx=10, pady=2, sticky="w")
            self.labels.append(lbl)

        self.tmp_label = ttk.Label(
            main_frame,
            text="Temperature on board (TMP1075): -- °C",
            font=("Segoe UI", 12, "bold")
        )
        self.tmp_label.pack(pady=5)

        self.btn = ttk.Button(main_frame, text="▶ Start", command=self.toggle)
        self.btn.pack(pady=10)

        # --- Wykres ---
        fig = Figure(figsize=(6, 3))
        self.ax = fig.add_subplot(111)
        self.ax.set_title("Temperatures over time")
        self.ax.set_xlabel("Time [samples]")
        self.ax.set_ylabel("°C")
        self.ax.yaxis.set_major_locator(MaxNLocator(nbins=18))

        self.lines = [self.ax.plot([], [], label=f"S{i+1}")[0] for i in range(10)]
        self.line_tmp, = self.ax.plot([], [], label="TMP1075",
                                      linewidth=2, color='black')
        self.ax.legend()

        self.canvas = FigureCanvasTkAgg(fig, master=main_frame)
        self.canvas.get_tk_widget().pack(fill="both", expand=True)

    def toggle(self):
        if self.running:
            self.running = False
            self.btn.config(text="▶ Start")
        else:
            self.running = True
            self.btn.config(text="⏹ Stop")
            threading.Thread(target=self.update_loop, daemon=True).start()

    def print_stats(self):
        if len(self.time_block) < 5:
            return

        def stats(name, buf):
            return (f"{name}: "
                    f"AVG={sum(buf)/len(buf)*1000:.2f} ms | "
                    f"MIN={min(buf)*1000:.2f} ms | "
                    f"MAX={max(buf)*1000:.2f} ms")

        print(stats("READ BLOCK (10)", self.time_block))
        print(stats("READ SINGLE", self.time_single))
        print(stats("FULL CYCLE", self.time_cycle))
        print("-" * 60)

    def update_loop(self):
        counter = 0

        while self.running:
            try:
                cycle_start = time.perf_counter()

                # --- BLOKOWY ODCZYT 10 REJESTRÓW ---
                t0 = time.perf_counter()
                raw = safe_read_registers(0, 10)
                t1 = time.perf_counter()
                self.time_block.append(t1 - t0)
                self.time_block = self.time_block[-300:]

                temperatures = [to_signed_16bit(x) / 100.0 for x in raw]

                # --- POJEDYNCZY REJESTR TMP ---
                t2 = time.perf_counter()
                tmp = safe_read_register(10)
                t3 = time.perf_counter()
                self.time_single.append(t3 - t2)
                self.time_single = self.time_single[-300:]

                tmp_temp = to_signed_16bit(tmp) / 100.0

                cycle_end = time.perf_counter()
                self.time_cycle.append(cycle_end - cycle_start)
                self.time_cycle = self.time_cycle[-300:]

                # --- GUI ---
                for i, t in enumerate(temperatures):
                    self.labels[i].config(
                        text=f"Sensor {i+1}: {t:.2f} °C")
                    self.data_history[i].append(t)
                    self.data_history[i] = self.data_history[i][-300:]

                self.tmp_label.config(
                    text=f"Temperature on board (TMP1075): {tmp_temp:.2f} °C")
                self.tmp_history.append(tmp_temp)
                self.tmp_history = self.tmp_history[-300:]

                for i, line in enumerate(self.lines):
                    line.set_data(range(len(self.data_history[i])),
                                  self.data_history[i])
                self.line_tmp.set_data(range(len(self.tmp_history)),
                                       self.tmp_history)

                self.ax.relim()
                self.ax.autoscale_view()
                self.canvas.draw()

                counter += 1
                if counter % 10 == 0:
                    self.print_stats()

            except Exception as e:
                print("Błąd odczytu:", e)

            # UWAGA: sleep NIE JEST liczony w pomiarach
            time.sleep(1)


# --- Start ---
if __name__ == "__main__":
    root = tk.Tk()
    app = ModbusGUI(root)
    root.mainloop()
