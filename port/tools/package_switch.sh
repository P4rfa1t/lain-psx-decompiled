#!/bin/bash
# Builds the Nintendo Switch homebrew release (lain.nro) with devkitPro's
# devkitA64 toolchain and Mesa's EGL/GLES3 port.
#   port/tools/package_switch.sh          -> dist/Lain-switch.zip
# The package contains no game data: players import their own discs on the SD card.
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p dist
STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT
cp -R port "$STAGE/port"
cp LICENSE "$STAGE/LICENSE"
cp -R thirdparty "$STAGE/thirdparty"
docker run --rm \
  -v "$STAGE/port":/src:ro \
  -v "$STAGE/LICENSE":/lic/LICENSE:ro \
  -v "$STAGE/thirdparty":/lic/thirdparty:ro \
  -v "$PWD/dist":/out \
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
    if [ -f /tmp/s/lain.nro ]; then
      cp /tmp/s/lain.nro /tmp/pkg/Lain/lain.nro
    else
      "$DEVKITPRO/tools/bin/nacptool" --create \
        "Serial Experiments Lain" "lain-psx-decompiled" "0.1.0" /tmp/s/lain.nacp
      "$DEVKITPRO/tools/bin/elf2nro" /tmp/s/lain.elf /tmp/pkg/Lain/lain.nro \
        --nacp=/tmp/s/lain.nacp
    fi
    cp /lic/LICENSE /tmp/pkg/Lain/LICENSE.txt
    cp -R /lic/thirdparty /tmp/pkg/Lain/thirdparty
    cat > /tmp/pkg/Lain/README.txt <<EOF
Serial Experiments Lain (Nintendo Switch)

Put lain.nro in sdmc:/switch/lain/ and start it from the Homebrew Menu.
Put your own disc dumps (cue + bin) in sdmc:/switch/lain/discs/, then use
"Add disc..." in the launcher. No game data is included.

Subtitle/dub packs: sdmc:/switch/lain/lang/<code>/ and sdmc:/switch/lain/dub/<code>/.
A Chinese UI font can be placed at sdmc:/switch/lain/fonts/ui.ttf.
EOF
    cd /tmp/pkg && rm -f /out/Lain-switch.zip && zip -qr /out/Lain-switch.zip Lain
  '
ls -la dist/Lain-switch.zip
