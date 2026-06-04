"""
Simulador del firmware LPC1769 para probar la GUI sin la placa.

Crea un puerto serial virtual (usando PTY) que responde exactamente
como el microcontrolador real.

Uso:
    python mock_lpc1769.py

Va a mostrar algo como:
    🎮 Mock LPC1769 listo en /dev/pts/5
Luego conectás la GUI a ese puerto.
"""

import os
import pty
import select
import time
import random
import threading

# ──────────────────────────────────────────────
# Estados del juego (igual que el firmware real)
# ──────────────────────────────────────────────

E_IDLE, E_CONFIG, E_COUNTDOWN, E_WAIT_GO, E_ROUND_END, E_GAME_OVER = range(6)

MAX_VICTORIAS = 10


class MockLPC1769:
    def __init__(self):
        self.estado = E_IDLE
        self.tecla_objetivo = "A"
        self.tono_frecuencia = 440
        self.velocidad_beeps = 150
        self.victorias_j1 = 0
        self.victorias_j2 = 0
        self.resultado_ronda = 0

    def procesar_comando(self, cmd: str):
        cmd = cmd.strip()
        if not cmd:
            return

        if cmd[0] == "S":
            self.estado = E_CONFIG
            self._enviar("S")

        elif cmd[0] == "K":
            if len(cmd) >= 2:
                self.tecla_objetivo = cmd[1].upper()
                self._enviar(f"K{self.tecla_objetivo}")

        elif cmd[0] == "F":
            try:
                val = int(cmd[1:].strip())
                if val > 0:
                    self.tono_frecuencia = val
                    self._enviar(f"F{self.tono_frecuencia}")
            except ValueError:
                pass

        elif cmd[0] == "B":
            try:
                val = int(cmd[1:].strip())
                self.velocidad_beeps = min(val, 200)
                self._enviar(f"B{self.velocidad_beeps}")
            except ValueError:
                pass

        elif cmd[0] == "C" and self.estado == E_CONFIG:
            self._enviar("C")
            # Simular countdown en un hilo
            threading.Thread(target=self._simular_ronda, daemon=True).start()

        elif cmd[0] == "?":
            self._enviar_scores()

        elif cmd[0] == "R":
            self.victorias_j1 = 0
            self.victorias_j2 = 0
            self._enviar("R")

    def _enviar(self, msg: str):
        data = (msg + "\r\n").encode()
        try:
            os.write(self.master_fd, data)
        except OSError:
            pass

    def _enviar_scores(self):
        self._enviar(f"S{self.victorias_j1:02d},{self.victorias_j2:02d}")

    def _simular_ronda(self):
        """Simula countdown → wait → resultado → scores → loop."""
        self.estado = E_COUNTDOWN

        # Countdown: 3...2...1...GO! (~2.5s simulado)
        time.sleep(1.0)
        self._enviar("3...")
        time.sleep(0.8)
        self._enviar("2...")
        time.sleep(0.8)
        self._enviar("1...")
        time.sleep(0.5)
        self._enviar("GO!")
        self.estado = E_WAIT_GO

        # Simular que alguien aprieta una tecla después de un tiempo aleatorio
        time.sleep(random.uniform(0.5, 2.0))

        # Decidir resultado
        self.estado = E_ROUND_END
        resultado = random.choices(
            [1, 2, 3, 4, 0],          # J1, J2, E1, E2, timeout
            weights=[35, 35, 10, 10, 10],
            k=1
        )[0]

        if resultado == 0:
            self._enviar("T")
        elif resultado == 1:
            self.victorias_j1 += 1
            reaccion = random.randint(80, 350)
            self._enviar(f"W1")
            self._enviar(f"R{reaccion}")
        elif resultado == 2:
            self.victorias_j2 += 1
            reaccion = random.randint(80, 350)
            self._enviar(f"W2")
            self._enviar(f"R{reaccion}")
        elif resultado == 3:
            if self.victorias_j1 > 0:
                self.victorias_j1 -= 1
            self._enviar("E1")
        elif resultado == 4:
            if self.victorias_j2 > 0:
                self.victorias_j2 -= 1
            self._enviar("E2")

        # Enviar scores
        self._enviar_scores()

        # Esperar 2 segundos como el firmware real
        time.sleep(2.0)

        # Verificar si alguien ganó la partida
        if self.victorias_j1 >= MAX_VICTORIAS:
            self._enviar("G1")
            self.estado = E_GAME_OVER
            time.sleep(1.0)
            self.estado = E_IDLE
        elif self.victorias_j2 >= MAX_VICTORIAS:
            self._enviar("G2")
            self.estado = E_GAME_OVER
            time.sleep(1.0)
            self.estado = E_IDLE
        else:
            # Siguiente ronda (vuelve a countdown, igual que firmware real)
            self.estado = E_COUNTDOWN
            # Auto-lanzar siguiente ronda
            threading.Thread(target=self._simular_ronda, daemon=True).start()

    def run(self):
        # Crear PTY: master = lo que escribe el mock, slave = "puerto serial"
        self.master_fd, slave_fd = pty.openpty()
        slave_name = os.ttyname(slave_fd)

        print(f"\n🎮 Mock LPC1769 listo en {slave_name}")
        print(f"   Conectá la GUI a ese puerto para probar.\n")
        print(f"   Comandos del mock (escribí en esta terminal):")
        print(f"     w1 → simular que J1 gana ronda")
        print(f"     w2 → simular que J2 gana ronda")
        print(f"     e1 → simular error de J1")
        print(f"     e2 → simular error de J2")
        print(f"     t  → simular timeout")
        print(f"     q  → salir\n")
        print(f"{"-" * 50}")

        # Buffer de lectura
        buf = ""
        stdin_buf = ""

        # Solo escuchar stdin si es una terminal real
        fd_list = [self.master_fd]
        if os.isatty(0):
            fd_list.append(0)

        try:
            while True:
                rlist, _, _ = select.select(fd_list, [], [], 0.1)

                for fd in rlist:
                    if fd == 0:
                        # Comandos desde la consola del mock
                        try:
                            ch = os.read(0, 64).decode("ascii", errors="replace")
                        except (OSError, EOFError):
                            continue
                        if not ch:
                            continue
                        for c in ch:
                            if c == '\n' or c == '\r':
                                if stdin_buf.strip():
                                    self._procesar_consola(stdin_buf)
                                stdin_buf = ""
                            else:
                                stdin_buf += c

                    elif fd == self.master_fd:
                        # Datos desde la GUI (comandos)
                        try:
                            data = os.read(self.master_fd, 64).decode("ascii", errors="replace")
                        except OSError:
                            data = ""

                        if not data:
                            continue

                        buf += data
                        while "\n" in buf:
                            line, buf = buf.split("\n", 1)
                            line = line.strip("\r ")
                            if line:
                                print(f">>> {line}")
                                self.procesar_comando(line)
        except KeyboardInterrupt:
            print("\n👋 Mock detenido")
        finally:
            os.close(self.master_fd)
            os.close(slave_fd)

    def _procesar_consola(self, line: str):
        """Comandos manuales para simular eventos desde la consola."""
        cmd = line.strip().lower()
        if cmd == "q":
            raise KeyboardInterrupt()
        elif cmd == "w1":
            self._enviar("W1")
            self.victorias_j1 += 1
            self._enviar(f"R{random.randint(80, 350)}")
            self._enviar_scores()
        elif cmd == "w2":
            self._enviar("W2")
            self.victorias_j2 += 1
            self._enviar(f"R{random.randint(80, 350)}")
            self._enviar_scores()
        elif cmd == "e1":
            if self.victorias_j1 > 0:
                self.victorias_j1 -= 1
            self._enviar("E1")
            self._enviar_scores()
        elif cmd == "e2":
            if self.victorias_j2 > 0:
                self.victorias_j2 -= 1
            self._enviar("E2")
            self._enviar_scores()
        elif cmd == "t":
            self._enviar("T")
            self._enviar_scores()
        elif cmd == "g1":
            self.victorias_j1 = MAX_VICTORIAS
            self._enviar("G1")
        elif cmd == "g2":
            self.victorias_j2 = MAX_VICTORIAS
            self._enviar("G2")
        else:
            print(f"  Comando desconocido: {line}")
            print(f"  Usá: w1, w2, e1, e2, t, g1, g2, q")


if __name__ == "__main__":
    mock = MockLPC1769()
    mock.run()
