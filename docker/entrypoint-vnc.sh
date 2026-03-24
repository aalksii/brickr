#!/usr/bin/env bash
set -euo pipefail

export DISPLAY="${DISPLAY:-:99}"
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/tmp/runtime-root}"
export BINVOX_PATH="${BINVOX_PATH:-/opt/brickr/tools/obj_to_binvox.py}"
export BRICKR_OPEN_FILE="${BRICKR_OPEN_FILE:-/opt/brickr/models/toyplane.obj}"
export BRICKR_VOX_RES="${BRICKR_VOX_RES:-30}"
export BRICKR_OUTPUT_DIR="${BRICKR_OUTPUT_DIR:-/opt/brickr/output}"
export LIBGL_ALWAYS_SOFTWARE="${LIBGL_ALWAYS_SOFTWARE:-1}"
export GALLIUM_DRIVER="${GALLIUM_DRIVER:-softpipe}"
export MESA_LOADER_DRIVER_OVERRIDE="${MESA_LOADER_DRIVER_OVERRIDE:-llvmpipe}"

mkdir -p "$XDG_RUNTIME_DIR"
chmod 700 "$XDG_RUNTIME_DIR"

Xvfb "$DISPLAY" -screen 0 1440x960x24 -ac +extension GLX +render -noreset &
XVFB_PID=$!

x11vnc -display "$DISPLAY" -forever -shared -nopw -listen 0.0.0.0 -xkb >/tmp/x11vnc.log 2>&1 &
X11VNC_PID=$!

websockify --web=/usr/share/novnc/ 6080 localhost:5900 >/tmp/websockify.log 2>&1 &
WEBSOCKIFY_PID=$!

sleep 2

/opt/brickr/brickr >/tmp/brickr.log 2>&1 &
BRICKR_PID=$!

cleanup() {
  kill "$BRICKR_PID" "$WEBSOCKIFY_PID" "$X11VNC_PID" "$XVFB_PID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

echo "Brickr desktop available at http://localhost:6080/vnc.html"
echo "VNC port available at localhost:5900"
echo "Startup mesh: $BRICKR_OPEN_FILE (resolution $BRICKR_VOX_RES)"
echo "Output directory: $BRICKR_OUTPUT_DIR"

wait "$BRICKR_PID"
