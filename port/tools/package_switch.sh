#!/bin/bash
# Builds the Nintendo Switch homebrew release (lain.nro) with devkitPro's
# devkitA64 toolchain and Mesa's EGL/GLES3 port.
#   port/tools/package_switch.sh          -> dist/Lain-switch.zip
# The package contains no game data: players import their own discs on the SD card.
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p dist build
# A folder inside the checkout (gitignored) is reliably shared with Docker
# Desktop; /tmp is not on some Windows setups.
STAGE="${LAIN_SWITCH_STAGE:-$PWD/build/switch-stage}"
rm -rf "$STAGE"
mkdir -p "$STAGE"
trap 'rm -rf "$STAGE"' EXIT
cp -R port "$STAGE/port"
cp LICENSE "$STAGE/port/LICENSE"
cp -R thirdparty "$STAGE/port/thirdparty"
# Docker Desktop on Windows needs C:/... paths; on Linux this is a no-op.
docker_path() {
  if command -v cygpath >/dev/null 2>&1; then cygpath -m "$1"; else printf '%s' "$1"; fi
}
OUT_NAME="${LAIN_SWITCH_ZIP:-Lain-switch.zip}"
docker run --rm -e OUT_NAME="$OUT_NAME" \
  -v "$(docker_path "$STAGE/port")":/src:ro \
  -v "$(docker_path "$PWD/dist")":/out \
  devkitpro/devkita64:latest bash -euc '
    dkp-pacman -S --needed --noconfirm \
      switch-dev switch-cmake switch-tools switch-pkg-config \
      switch-sdl2 switch-mesa switch-libdrm_nouveau >/dev/null
    cmake -S /src -B /tmp/s -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/Switch.cmake" \
      -DCMAKE_BUILD_TYPE=Release \
      -DLAIN_STATIC_DEPS=OFF \
      -DPython3_EXECUTABLE=/usr/bin/python3 \
      -Wno-dev >/tmp/configure.log 2>&1 || { cat /tmp/configure.log; exit 1; }
    cmake --build /tmp/s >/tmp/build.log 2>&1 || { tail -80 /tmp/build.log; exit 1; }
    mkdir -p /tmp/pkg/Lain
    # Always build the NRO here so the icon and NACP are the packaged ones.
    "$DEVKITPRO/tools/bin/nacptool" --create \
      "Serial Experiments Lain" "lain-psx-decompiled" "0.1.0" /tmp/s/lain.nacp
    ICON=""
    if [ -f /src/src/lain.jpg ]; then
      ICON="--icon=/src/src/lain.jpg"
    fi
    "$DEVKITPRO/tools/bin/elf2nro" /tmp/s/lain.elf /tmp/pkg/Lain/lain.nro \
      --nacp=/tmp/s/lain.nacp $ICON
    cp /src/LICENSE /tmp/pkg/Lain/LICENSE.txt
    cp -R /src/thirdparty /tmp/pkg/Lain/thirdparty
    cat > /tmp/pkg/Lain/README.txt <<EOF
Serial Experiments Lain (Nintendo Switch)

Copy everything in this folder into sdmc:/switch/lain/ and start lain.nro
from the Homebrew Menu.

Discs: put your own .cue/.bin/.img dumps in sdmc:/switch/lain/discs/. The
client verifies them and uses them in place - no copy and no import step.
No game data is included.

The Chinese UI font, the launcher artwork and the Chinese/English subtitle
packs are built into lain.nro. On the first start they are written to
sdmc:/switch/lain/ (fonts/, lang/, lain_logo.rgba); edit them there freely.
EOF
    cd /tmp/pkg && rm -f "/out/$OUT_NAME" && zip -qr "/out/$OUT_NAME" Lain
  '
ls -la "dist/$OUT_NAME"
