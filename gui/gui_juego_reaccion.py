#!/usr/bin/env python3
"""
gui_juego_reaccion.py

Interfaz gráfica en Python (tkinter) para el "Juego de reacción
competitivo" basado en LPC1769 - adaptada al protocolo del Paso 6
(sistema de puntaje + condición de victoria).

Protocolo UART (115200 8N1, terminado en \\r\\n):

  Enviados por la PC hacia la LPC1769:
    S\\r\\n       -> Inicia una ronda (la LPC mostrará "Presiona: X" y
                    esperará que algún jugador presione su teclado)
    K<c>\\r\\n     -> Configura la tecla objetivo a <c> (ej: "K7\\r\\n").
                    Tiene efecto en la PRÓXIMA ronda que se inicie con S.
    R\\r\\n       -> Reinicia el marcador a 0-0 (la LPC responde "P0,0")

  Recibidos desde la LPC1769:
    W1\\r\\n              -> Jugador 1 acertó la tecla objetivo
    W2\\r\\n              -> Jugador 2 acertó la tecla objetivo
    E1\\r\\n              -> Jugador 1 presionó una tecla incorrecta
    E2\\r\\n              -> Jugador 2 presionó una tecla incorrecta
    T\\r\\n               -> Timeout (nadie presionó en ~10s)
    R<tiempo_us>\\r\\n     -> Tiempo de reacción en microsegundos del
                           jugador que acertó (solo tras W1/W2)
    P<j1>,<j2>\\r\\n       -> Marcador actualizado (fuente de verdad,
                           se recibe tras cada ronda y tras "R")
    G1\\r\\n / G2\\r\\n     -> Jugador 1/2 ganó la partida completa
                           (el marcador se reinicia a 0-0 en la LPC,
                           que enviará "P0,0" a continuación)

Requiere:
    pip install pyserial
"""

import threading
import queue
import tkinter as tk
from tkinter import ttk, messagebox

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    serial = None


# --------------------------------------------------------------------------- #
#  Backend de comunicación serie
# --------------------------------------------------------------------------- #
class SerialWorker(threading.Thread):
    """
    Hilo dedicado a leer del puerto serie en background y volcar las
    líneas recibidas a una cola, para no bloquear la GUI.
    """

    def __init__(self, port: str, baudrate: int, rx_queue: queue.Queue):
        super().__init__(daemon=True)
        self.port = port
        self.baudrate = baudrate
        self.rx_queue = rx_queue
        self._stop_event = threading.Event()
        self.ser = None

    def run(self):
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=0.2)
        except Exception as exc:
            self.rx_queue.put(("error", f"No se pudo abrir el puerto: {exc}"))
            return

        self.rx_queue.put(("status", f"Conectado a {self.port} @ {self.baudrate}"))

        buf = b""
        while not self._stop_event.is_set():
            try:
                data = self.ser.read(64)
            except Exception as exc:
                self.rx_queue.put(("error", f"Error de lectura: {exc}"))
                break

            if data:
                buf += data
                while b"\n" in buf or b"\r" in buf:
                    # Cortar en el primer \r o \n que aparezca
                    idx_r = buf.find(b"\r")
                    idx_n = buf.find(b"\n")
                    if idx_r == -1:
                        idx = idx_n
                    elif idx_n == -1:
                        idx = idx_r
                    else:
                        idx = min(idx_r, idx_n)

                    line = buf[:idx]
                    buf = buf[idx + 1:]

                    if line:
                        try:
                            self.rx_queue.put(("rx", line.decode(errors="replace")))
                        except Exception:
                            pass

        if self.ser and self.ser.is_open:
            self.ser.close()
        self.rx_queue.put(("status", "Desconectado"))

    def send(self, text: str):
        """Envía una línea de texto terminada en \\r\\n por el puerto serie."""
        if self.ser and self.ser.is_open:
            try:
                self.ser.write((text + "\r\n").encode())
            except Exception as exc:
                self.rx_queue.put(("error", f"Error de escritura: {exc}"))

    def stop(self):
        self._stop_event.set()


# --------------------------------------------------------------------------- #
#  Interfaz gráfica
# --------------------------------------------------------------------------- #
class JuegoReaccionGUI(tk.Tk):

    TECLAS_VALIDAS = list("123456789ABCD*0#")
    META_VICTORIAS = 3

    def __init__(self):
        super().__init__()

        self.title("Juego de Reacción - Control PC")
        self.geometry("480x520")
        self.resizable(False, False)

        self.rx_queue = queue.Queue()
        self.worker = None

        self._build_ui()
        self._poll_queue()

    # ------------------------------------------------------------------- #
    #  Construcción de la UI
    # ------------------------------------------------------------------- #
    def _build_ui(self):
        pad = {"padx": 8, "pady": 4}

        # --- Sección Conexión ---
        frame_conn = ttk.LabelFrame(self, text="Conexión Serial")
        frame_conn.pack(fill="x", **pad)

        ttk.Label(frame_conn, text="Puerto:").grid(row=0, column=0, sticky="w", **pad)
        self.combo_port = ttk.Combobox(frame_conn, width=20, state="readonly")
        self.combo_port.grid(row=0, column=1, **pad)
        self._refresh_ports()

        ttk.Button(frame_conn, text="Actualizar", command=self._refresh_ports).grid(
            row=0, column=2, **pad
        )

        ttk.Label(frame_conn, text="Baudrate:").grid(row=1, column=0, sticky="w", **pad)
        self.combo_baud = ttk.Combobox(
            frame_conn, width=20, state="readonly",
            values=["115200", "9600", "57600", "38400"]
        )
        self.combo_baud.set("115200")
        self.combo_baud.grid(row=1, column=1, **pad)

        self.btn_connect = ttk.Button(frame_conn, text="Conectar", command=self._toggle_connection)
        self.btn_connect.grid(row=1, column=2, **pad)

        self.lbl_status = ttk.Label(frame_conn, text="Desconectado", foreground="red")
        self.lbl_status.grid(row=2, column=0, columnspan=3, sticky="w", **pad)

        # --- Sección Configuración de tecla objetivo ---
        frame_cfg = ttk.LabelFrame(self, text="Tecla objetivo (comando K)")
        frame_cfg.pack(fill="x", **pad)

        ttk.Label(frame_cfg, text="Tecla:").grid(row=0, column=0, sticky="w", **pad)
        self.combo_tecla = ttk.Combobox(
            frame_cfg, width=8, state="readonly",
            values=self.TECLAS_VALIDAS
        )
        self.combo_tecla.set("5")
        self.combo_tecla.grid(row=0, column=1, sticky="w", **pad)

        ttk.Button(frame_cfg, text="Aplicar tecla objetivo (K)",
                   command=self._send_k).grid(row=0, column=2, sticky="we", **pad)

        ttk.Label(
            frame_cfg,
            text="Nota: el cambio de tecla objetivo se aplica recién\n"
                 "en la PRÓXIMA ronda que se inicie con 'Iniciar Ronda'.",
            foreground="gray"
        ).grid(row=1, column=0, columnspan=3, sticky="w", **pad)

        # --- Sección Control de juego ---
        frame_ctrl = ttk.LabelFrame(self, text="Control de juego")
        frame_ctrl.pack(fill="x", **pad)

        ttk.Button(frame_ctrl, text="Iniciar Ronda (S)",
                   command=lambda: self._send_line("S")).grid(
            row=0, column=0, columnspan=3, sticky="we", **pad
        )
        frame_ctrl.columnconfigure(0, weight=1)

        # --- Sección Estado / Resultado ---
        frame_result = ttk.LabelFrame(self, text="Resultado de la última ronda")
        frame_result.pack(fill="x", **pad)

        self.lbl_evento = ttk.Label(frame_result, text="Sin rondas todavía",
                                     font=("TkDefaultFont", 13, "bold"))
        self.lbl_evento.grid(row=0, column=0, sticky="w", padx=20, pady=8)

        self.lbl_tiempo = ttk.Label(frame_result, text="Tiempo de reacción: -- µs")
        self.lbl_tiempo.grid(row=1, column=0, sticky="w", padx=20, pady=4)

        # --- Sección Marcador ---
        frame_score = ttk.LabelFrame(self, text="Marcador (sincronizado con la LPC)")
        frame_score.pack(fill="x", **pad)

        self.lbl_j1 = ttk.Label(frame_score, text="Jugador 1: 0", font=("TkDefaultFont", 14, "bold"))
        self.lbl_j1.grid(row=0, column=0, sticky="w", padx=20, pady=8)

        self.lbl_j2 = ttk.Label(frame_score, text="Jugador 2: 0", font=("TkDefaultFont", 14, "bold"))
        self.lbl_j2.grid(row=0, column=1, sticky="w", padx=20, pady=8)

        ttk.Label(frame_score, text=f"Meta: {self.META_VICTORIAS} puntos para ganar la partida",
                  foreground="gray").grid(row=1, column=0, columnspan=2, sticky="w", padx=20, pady=2)

        ttk.Button(frame_score, text="Reiniciar marcador (R)",
                   command=lambda: self._send_line("R")).grid(
            row=2, column=0, columnspan=2, sticky="we", padx=20, pady=4
        )

        # --- Consola de mensajes ---
        frame_log = ttk.LabelFrame(self, text="Mensajes (crudo)")
        frame_log.pack(fill="both", expand=True, **pad)

        self.text_log = tk.Text(frame_log, height=8, state="disabled", wrap="word")
        self.text_log.pack(fill="both", expand=True, padx=4, pady=4)

    # ------------------------------------------------------------------- #
    #  Helpers de conexión
    # ------------------------------------------------------------------- #
    def _refresh_ports(self):
        if serial is None:
            self.combo_port["values"] = []
            return
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.combo_port["values"] = ports
        if ports:
            self.combo_port.current(0)

    def _toggle_connection(self):
        if self.worker is None:
            self._connect()
        else:
            self._disconnect()

    def _connect(self):
        if serial is None:
            messagebox.showerror(
                "Falta dependencia",
                "Necesitás instalar pyserial:\n\n    pip install pyserial"
            )
            return

        port = self.combo_port.get()
        baud = self.combo_baud.get()

        if not port:
            messagebox.showwarning("Puerto no seleccionado", "Elegí un puerto serie.")
            return

        try:
            baudrate = int(baud)
        except ValueError:
            messagebox.showerror("Baudrate inválido", "El baudrate debe ser un número.")
            return

        self.worker = SerialWorker(port, baudrate, self.rx_queue)
        self.worker.start()
        self.btn_connect.config(text="Desconectar")

    def _disconnect(self):
        if self.worker:
            self.worker.stop()
            self.worker = None
        self.btn_connect.config(text="Conectar")
        self.lbl_status.config(text="Desconectado", foreground="red")

    # ------------------------------------------------------------------- #
    #  Envío de comandos
    # ------------------------------------------------------------------- #
    def _send_line(self, text: str):
        if self.worker is None:
            messagebox.showwarning("Sin conexión", "Conectate al puerto serie primero.")
            return
        self.worker.send(text)
        self._log(f">> {text}")

    def _send_k(self):
        tecla = self.combo_tecla.get().strip()
        if not tecla:
            messagebox.showwarning("Falta tecla", "Elegí una tecla objetivo.")
            return
        self._send_line(f"K{tecla}")

    # ------------------------------------------------------------------- #
    #  Procesamiento de mensajes recibidos
    # ------------------------------------------------------------------- #
    def _poll_queue(self):
        try:
            while True:
                kind, payload = self.rx_queue.get_nowait()
                if kind == "status":
                    color = "green" if "Conectado" in payload else "red"
                    self.lbl_status.config(text=payload, foreground=color)
                elif kind == "error":
                    self._log(f"[ERROR] {payload}")
                    messagebox.showerror("Error de comunicación", payload)
                    self._disconnect()
                elif kind == "rx":
                    self._log(f"<< {payload}")
                    self._handle_message(payload)
        except queue.Empty:
            pass

        self.after(100, self._poll_queue)

    def _handle_message(self, msg: str):
        if not msg:
            return

        if msg == "T":
            self.lbl_evento.config(text="TIEMPO AGOTADO", foreground="black")
            self.lbl_tiempo.config(text="Tiempo de reacción: -- µs")

        elif msg == "W1":
            self.lbl_evento.config(text="¡Jugador 1 acertó!", foreground="green")

        elif msg == "W2":
            self.lbl_evento.config(text="¡Jugador 2 acertó!", foreground="green")

        elif msg == "E1":
            self.lbl_evento.config(text="Jugador 1 falló (tecla incorrecta)", foreground="red")
            self.lbl_tiempo.config(text="Tiempo de reacción: -- µs")

        elif msg == "E2":
            self.lbl_evento.config(text="Jugador 2 falló (tecla incorrecta)", foreground="red")
            self.lbl_tiempo.config(text="Tiempo de reacción: -- µs")

        elif msg.startswith("R") and msg[1:].isdigit():
            self.lbl_tiempo.config(text=f"Tiempo de reacción: {msg[1:]} µs")

        elif msg.startswith("P") and "," in msg:
            try:
                j1_str, j2_str = msg[1:].split(",")
                self.lbl_j1.config(text=f"Jugador 1: {int(j1_str)}")
                self.lbl_j2.config(text=f"Jugador 2: {int(j2_str)}")
            except ValueError:
                pass

        elif msg == "G1":
            self.lbl_evento.config(text="¡JUGADOR 1 GANÓ LA PARTIDA!", foreground="green")
            messagebox.showinfo("Fin de partida", "¡Jugador 1 ha ganado la partida!\nEl marcador se reinicia.")

        elif msg == "G2":
            self.lbl_evento.config(text="¡JUGADOR 2 GANÓ LA PARTIDA!", foreground="green")
            messagebox.showinfo("Fin de partida", "¡Jugador 2 ha ganado la partida!\nEl marcador se reinicia.")

    # ------------------------------------------------------------------- #
    #  Logging
    # ------------------------------------------------------------------- #
    def _log(self, text: str):
        self.text_log.config(state="normal")
        self.text_log.insert("end", text + "\n")
        self.text_log.see("end")
        self.text_log.config(state="disabled")

    # ------------------------------------------------------------------- #
    #  Cierre de la app
    # ------------------------------------------------------------------- #
    def destroy(self):
        if self.worker:
            self.worker.stop()
        super().destroy()


if __name__ == "__main__":
    app = JuegoReaccionGUI()
    app.mainloop()
