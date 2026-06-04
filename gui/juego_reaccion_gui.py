"""
Interfaz gráfica mínima para el Juego de Reacción Competitivo (LPC1769).
Se comunica por UART con el microcontrolador.

Uso:
    1. Conectar la placa LPC1769 por USB
    2. Ver qué COM asignó (Administrador de Dispositivos)
    3. python juego_reaccion_gui.py
    4. Seleccionar COM, conectar, y usar los botones
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import threading

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    messagebox.showerror(
        "Falta pyserial",
        "Instalá pyserial con:\n\n  pip install pyserial\n\n"
        "o si usás Python desde MCUXpresso:\n  python -m pip install pyserial",
    )
    raise SystemExit(1)


# ──────────────────────────────────────────────
# Constantes del protocolo
# ──────────────────────────────────────────────

BAUDRATE = 115200
TIMEOUT = 0.1  # segundos para lectura no bloqueante

# Límites del firmware
FREC_MIN = 20       # Hz — audible
FREC_MAX = 5000     # Hz — Nyquist para sample rate 10 kHz
BEEP_MIN = 1        # ms entre beeps
BEEP_MAX = 200      # ms — tope del firmware


class ReaccionGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Juego de Reacción Competitivo")
        self.root.resizable(False, False)

        self.ser = None
        self.reader_active = False

        self._build_widgets()
        self._refresh_ports()

    # ────────── Construcción de widgets ──────────

    def _build_widgets(self):
        main = ttk.Frame(self.root, padding=12)
        main.grid(row=0, column=0)

        # ── Fila 0: Conexión ──
        conn_frame = ttk.LabelFrame(main, text="Conexión", padding=8)
        conn_frame.grid(row=0, column=0, columnspan=4, sticky="ew", pady=(0, 8))

        ttk.Label(conn_frame, text="Puerto:").grid(row=0, column=0, padx=(0, 4))
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(
            conn_frame, textvariable=self.port_var, width=16
        )
        self.port_combo.grid(row=0, column=1, padx=(0, 8))

        self.refresh_btn = ttk.Button(
            conn_frame, text="⟳", width=3, command=self._refresh_ports
        )
        self.refresh_btn.grid(row=0, column=2, padx=(0, 8))

        self.connect_btn = ttk.Button(
            conn_frame, text="Conectar", command=self._toggle_connection
        )
        self.connect_btn.grid(row=0, column=3)

        # ── Fila 1: Configuración ──
        cfg_frame = ttk.LabelFrame(main, text="Configuración", padding=8)
        cfg_frame.grid(row=1, column=0, columnspan=4, sticky="ew", pady=(0, 8))

        # ── Fila 0: Tecla objetivo ──
        ttk.Label(cfg_frame, text="Tecla:").grid(row=0, column=0, padx=(0, 4))
        self.tecla_var = tk.StringVar(value="*")
        TECLAS_MATRIZ = ["*", "#", "0", "1", "2", "3", "4", "5",
                          "6", "7", "8", "9", "A", "B", "C", "D"]
        tecla_menu = ttk.Combobox(
            cfg_frame, textvariable=self.tecla_var,
            values=TECLAS_MATRIZ, width=4, state="readonly",
        )
        tecla_menu.grid(row=0, column=1, padx=(0, 8))
        ttk.Button(
            cfg_frame, text="Enviar K", command=self._cmd_tecla
        ).grid(row=0, column=2, padx=(0, 4))

        # ── Fila 0: Frecuencia ──
        ttk.Label(cfg_frame, text="Frec (Hz):").grid(row=0, column=4, padx=(20, 4))
        self.freq_var = tk.StringVar(value="440")
        freq_entry = ttk.Entry(cfg_frame, textvariable=self.freq_var, width=6)
        freq_entry.grid(row=0, column=5, padx=(0, 2))
        ttk.Label(cfg_frame, text=f"({FREC_MIN}-{FREC_MAX})").grid(row=0, column=6, padx=(0, 8))
        ttk.Button(
            cfg_frame, text="Enviar F", command=self._cmd_frecuencia
        ).grid(row=0, column=7, padx=(0, 4))

        # ── Fila 1: Velocidad beeps ──
        ttk.Label(cfg_frame, text="Beeps (ms):").grid(row=1, column=0, padx=(0, 4), pady=(4, 0))
        self.bpm_var = tk.StringVar(value="150")
        bpm_entry = ttk.Entry(cfg_frame, textvariable=self.bpm_var, width=4)
        bpm_entry.grid(row=1, column=1, padx=(0, 2), pady=(4, 0))
        ttk.Label(cfg_frame, text=f"(ms entre beeps, {BEEP_MIN}-{BEEP_MAX})").grid(
            row=1, column=2, columnspan=2, padx=(0, 4), pady=(4, 0), sticky="w"
        )
        ttk.Button(
            cfg_frame, text="Enviar B", command=self._cmd_beeps
        ).grid(row=1, column=4, padx=(0, 4), pady=(4, 0))

        # ── Fila 2: Acciones ──
        action_frame = ttk.LabelFrame(main, text="Acciones", padding=8)
        action_frame.grid(row=2, column=0, columnspan=4, sticky="ew", pady=(0, 8))

        ttk.Button(action_frame, text="▶ Iniciar (S)", command=self._cmd_start).grid(
            row=0, column=0, padx=4
        )
        ttk.Button(action_frame, text="✔ Confirmar (C)", command=self._cmd_confirm).grid(
            row=0, column=1, padx=4
        )
        ttk.Button(action_frame, text="❓ Scores (?)", command=self._cmd_query).grid(
            row=0, column=2, padx=4
        )
        ttk.Button(action_frame, text="⟲ Reset (R)", command=self._cmd_reset).grid(
            row=0, column=3, padx=4
        )

        # ── Fila 3: Scores ──
        score_frame = ttk.LabelFrame(main, text="Puntaje", padding=8)
        score_frame.grid(row=3, column=0, columnspan=4, sticky="ew", pady=(0, 8))

        self.score_var = tk.StringVar(value="J1: 0  —  J2: 0")
        ttk.Label(score_frame, textvariable=self.score_var, font=("Consolas", 20, "bold")).pack()

        # ── Fila 4: Log ──
        log_frame = ttk.LabelFrame(main, text="Mensajes del LPC1769", padding=8)
        log_frame.grid(row=4, column=0, columnspan=4, sticky="ew")

        self.log = scrolledtext.ScrolledText(
            log_frame, width=60, height=12, font=("Consolas", 10), state="disabled"
        )
        self.log.pack()

    # ────────── Puerto serie ──────────

    def _refresh_ports(self):
        ports = serial.tools.list_ports.comports()
        # Filtra ttyS* (puertos serie internos de la mother, no se usan)
        items = [f"{p.device} — {p.description}" for p in ports
                 if not p.device.startswith("/dev/ttyS")]
        # Marcador para escribir a mano (mock, etc.)
        items.append("✎ Escribir path manual...")
        self.port_combo["values"] = items
        # Buscar el primer puerto USB/ACM real
        default = next((it for it in items if "/dev/ttyUSB" in it or "/dev/ttyACM" in it), None)
        if default:
            self.port_var.set(default)
        elif items:
            self.port_combo.current(0)
        else:
            self.port_var.set("")

    def _toggle_connection(self):
        if self.ser and self.ser.is_open:
            self._disconnect()
        else:
            self._connect()

    def _connect(self):
        raw = self.port_var.get().strip()
        if not raw or raw.startswith("✎"):
            messagebox.showwarning(
                "Puerto inválido",
                "Escribí el path del puerto directamente,\nej: /dev/ttyUSB0, COM3, /dev/pts/5\n\n"
                "En Linux con la placa conectada suele ser /dev/ttyUSB0\n"
                "Con el mock: /dev/pts/X (te lo dice el mock)",
            )
            return
        port = raw.split(" ")[0]  # "COM3 — descripción" -> "COM3"

        try:
            self.ser = serial.Serial(port, BAUDRATE, timeout=TIMEOUT)
        except serial.SerialException as e:
            messagebox.showerror("Error", f"No se pudo abrir {port}:\n{e}")
            return

        self.connect_btn.config(text="Desconectar")
        self.reader_active = True
        threading.Thread(target=self._reader_loop, daemon=True).start()
        self._log(f"Conectado a {port} a {BAUDRATE} baud")

    def _disconnect(self):
        self.reader_active = False
        if self.ser:
            try:
                self.ser.close()
            except Exception:
                pass
            self.ser = None
        self.connect_btn.config(text="Conectar")
        self._log("Desconectado")

    # ────────── Envío de comandos ──────────

    def _send(self, cmd: str):
        if not self.ser or not self.ser.is_open:
            messagebox.showwarning("Desconectado", "Conectá al puerto primero")
            return
        data = (cmd + "\r\n").encode()
        self.ser.write(data)
        self._log(f">>> {cmd}")

    def _cmd_start(self):
        self._send("S")

    def _cmd_tecla(self):
        self._send(f"K{self.tecla_var.get()}")

    def _cmd_frecuencia(self):
        try:
            val = int(self.freq_var.get().strip())
        except ValueError:
            messagebox.showwarning("Frecuencia inválida", f"Tiene que ser un número entre {FREC_MIN} y {FREC_MAX}")
            return
        if val < FREC_MIN or val > FREC_MAX:
            messagebox.showwarning("Frecuencia fuera de rango", f"Usá un valor entre {FREC_MIN} y {FREC_MAX} Hz")
            return
        self._send(f"F{val}")

    def _cmd_beeps(self):
        try:
            val = int(self.bpm_var.get().strip())
        except ValueError:
            messagebox.showwarning("Beeps inválido", f"Tiene que ser un número entre {BEEP_MIN} y {BEEP_MAX}")
            return
        if val < BEEP_MIN or val > BEEP_MAX:
            messagebox.showwarning("Beeps fuera de rango", f"Usá un valor entre {BEEP_MIN} y {BEEP_MAX} ms")
            return
        self._send(f"B{val}")

    def _cmd_confirm(self):
        self._send("C")

    def _cmd_query(self):
        self._send("?")

    def _cmd_reset(self):
        self._send("R")

    # ────────── Lectura y parseo ──────────

    def _reader_loop(self):
        buf = ""
        while self.reader_active and self.ser and self.ser.is_open:
            try:
                data = self.ser.read(64).decode("ascii", errors="replace")
            except Exception:
                break
            if not data:
                continue
            buf += data

            while "\n" in buf:
                line, buf = buf.split("\n", 1)
                line = line.strip("\r ")
                if line:
                    self.root.after(0, self._handle_message, line)

    def _handle_message(self, msg: str):
        self._log(f"<<< {msg}")

        # Parsear mensajes conocidos
        if msg.startswith("S"):
            # SXX,YY
            try:
                parts = msg[1:].split(",")
                if len(parts) == 2:
                    j1 = int(parts[0])
                    j2 = int(parts[1])
                    self.score_var.set(f"J1: {j1}  —  J2: {j2}")
            except ValueError:
                pass

        elif msg == "W1":
            self._log("🎯 ¡J1 ganó la ronda!")
        elif msg == "W2":
            self._log("🎯 ¡J2 ganó la ronda!")
        elif msg == "E1":
            self._log("❌ J1 se equivocó")
        elif msg == "E2":
            self._log("❌ J2 se equivocó")
        elif msg == "T":
            self._log("⏰ Tiempo agotado")
        elif msg == "G1":
            self._log("🏆 ¡J1 ganó la partida!")
        elif msg == "G2":
            self._log("🏆 ¡J2 ganó la partida!")

    def _log(self, text: str):
        self.log.config(state="normal")
        self.log.insert("end", text + "\n")
        self.log.see("end")
        self.log.config(state="disabled")


# ──────────────────────────────────────────────
# Entry point
# ──────────────────────────────────────────────

if __name__ == "__main__":
    root = tk.Tk()
    app = ReaccionGUI(root)
    root.mainloop()
