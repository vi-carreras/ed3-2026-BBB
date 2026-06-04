#!/usr/bin/env bash
# Lanza el mock del LPC1769 y la GUI para probar sin la placa.
# Usá dos terminales separadas:
#
# Terminal 1:
#   bash gui/probar_mock.sh
#
# Terminal 2:
#   python3 gui/juego_reaccion_gui.py
#
# El mock te va a decir en qué /dev/pts/X está escuchando.
# En la GUI escribís ese path como puerto y conectás.
#
# También podés abrir una tercera terminal para mandar comandos
# manuales al mock (w1, w2, e1, e2, t, q).

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
echo "🎮 Mock LPC1769 — simulador del firmware"
echo "   Conectá la GUI al puerto que aparece abajo."
echo "   Directorio del repo: $SCRIPT_DIR"
echo ""
python3 "$SCRIPT_DIR/gui/mock_lpc1769.py"
