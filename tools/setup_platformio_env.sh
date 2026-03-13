#!/usr/bin/env bash
set -euo pipefail

python3 -m pip install --upgrade pip
python3 -m pip install platformio scons

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
XTC_DIR="$ROOT_DIR/tools/pio-packages/toolchain-xtensa-esp-elf"
XTC_URL="https://github.com/espressif/crosstool-NG/releases/download/esp-13.2.0_20240530/xtensa-esp-elf-13.2.0_20240530-x86_64-linux-gnu.tar.xz"

mkdir -p "$XTC_DIR"
if [[ ! -x "$XTC_DIR/bin/xtensa-esp32-elf-g++" ]]; then
  echo "Bootstrapping local toolchain-xtensa-esp-elf fallback package..."
  TMP_ARCHIVE="$(mktemp --suffix=.tar.xz)"
  TMP_EXTRACT_DIR="$(mktemp -d)"
  trap 'rm -f "$TMP_ARCHIVE"; rm -rf "$TMP_EXTRACT_DIR"' EXIT

  python3 - <<PY
import urllib.request
urllib.request.urlretrieve("$XTC_URL", "$TMP_ARCHIVE")
print("Downloaded:", "$TMP_ARCHIVE")
PY

  tar -xJf "$TMP_ARCHIVE" -C "$TMP_EXTRACT_DIR"
  INNER_DIR="$(find "$TMP_EXTRACT_DIR" -mindepth 1 -maxdepth 1 -type d | head -n 1)"
  rm -rf "$XTC_DIR/bin" "$XTC_DIR/lib" "$XTC_DIR/include" "$XTC_DIR/share" "$XTC_DIR/libexec" "$XTC_DIR/xtensa-esp-elf"
  cp -a "$INNER_DIR"/* "$XTC_DIR"/
fi

cat > "$XTC_DIR/package.json" <<'JSON'
{
  "name": "toolchain-xtensa-esp-elf",
  "version": "13.2.0+20240530",
  "description": "Local fallback Xtensa toolchain package for proxy-constrained builds",
  "system": ["linux_x86_64"]
}
JSON

echo "PlatformIO: $(python3 -m platformio --version)"
python3 - <<'PY'
import importlib.util
print('SCons available:', bool(importlib.util.find_spec('SCons')))
PY
