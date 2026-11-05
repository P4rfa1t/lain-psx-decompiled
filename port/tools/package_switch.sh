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
STAGE="$PWD/build/switch-stage"
rm -rf "$STAGE"
mkdir -p "$STAGE"
trap 'rm -rf "$STAGE"' EXIT
cp -R port "$STAGE/port"
cp LICENSE "$STAGE/LICENSE"
cp -R thirdparty "$STAGE/thirdparty"
# Docker Desktop on Windows needs C:/... paths; on Linux this is a no-op.
docker_path() {
  if command -v cygpath >/dev/null 2>&1; then cygpath -m "$1"; else printf '%s' "$1"; fi
}
docker run --rm \
  -v "$(docker_path "$STAGE/port")":/src:ro \
  -v "$(docker_path "$STAGE/LICENSE")":/lic/LICENSE:ro \
  -v "$(docker_path "$STAGE/thirdparty")":/lic/thirdparty:ro \
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
    # CJK UI font (WenQuanYi Micro Hei, Apache-2.0/GPLv3+exception), so Chinese
    # text does not fall back to ImGui's Latin-only font.
    mkdir -p /tmp/pkg/Lain/fonts
    if [ -f /src/fonts/ui.ttf ]; then
      cp /src/fonts/ui.ttf /tmp/pkg/Lain/fonts/ui.ttf
    else
      curl -L --fail --retry 3 -o /tmp/pkg/Lain/fonts/ui.ttf \
        https://raw.githubusercontent.com/anthonyfok/fonts-wqy-microhei/master/wqy-microhei.ttc
    fi
    cp /src/fonts/LICENSE_*.txt /src/fonts/README-WQY.txt /tmp/pkg/Lain/fonts/ 2>/dev/null || true
    cp /lic/LICENSE /tmp/pkg/Lain/LICENSE.txt
    cp -R /lic/thirdparty /tmp/pkg/Lain/thirdparty
    # Optional subtitle/dub packs staged in port/packs/<code>/ (gitignored, so
    # no third-party translation enters the repository or CI artifacts).
    if [ -d /src/packs ]; then
      mkdir -p /tmp/pkg/Lain/lang
      cp -R /src/packs/. /tmp/pkg/Lain/lang/
      printf "\nSubtitle packs are included under lang/.\n" >> /tmp/pkg/Lain/README.txt
    fi
    cat > /tmp/pkg/Lain/README.txt <<EOF
Serial Experiments Lain (Nintendo Switch)

Copy everything in this folder into sdmc:/switch/lain/ and start lain.nro
from the Homebrew Menu.

Discs: put your own .cue/.bin/.img dumps in sdmc:/switch/lain/discs/. The
client verifies them and uses them in place - no copy and no import step.
No game data is included.

Subtitle/dub packs: sdmc:/switch/lain/lang/<code>/ and sdmc:/switch/lain/dub/<code>/.
Chinese UI font: sdmc:/switch/lain/fonts/ui.ttf (included in this package).
EOF
    cd /tmp/pkg && rm -f /out/Lain-switch.zip && zip -qr /out/Lain-switch.zip Lain
  '
ls -la dist/Lain-switch.zip
