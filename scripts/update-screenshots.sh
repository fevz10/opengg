#!/usr/bin/env bash
# Regenerates the README screenshots (docs/screenshots/*.png) from the demo
# mode of the application, rendered offscreen so no display is needed.
#
# Usage:
#   ./scripts/update-screenshots.sh            # uses ./build (configures/builds it if needed)
#   BUILD_DIR=/path/to/build ./scripts/update-screenshots.sh
#
# If pngquant is installed the images are additionally palette-optimised
# (lossless-looking, ~60 % smaller) and stripped of all metadata.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build}"
OUT_DIR="$ROOT_DIR/docs/screenshots"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

if [[ ! -x "$BUILD_DIR/opengg" ]]; then
    echo ">> Building opengg in $BUILD_DIR"
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$BUILD_DIR" --parallel "$(nproc 2>/dev/null || echo 2)"
fi

echo ">> Rendering pages (demo mode, offscreen)"
echo '{"app":{"notifications":false}}' > "$TMP_DIR/config.json"
QT_QPA_PLATFORM=offscreen "$BUILD_DIR/opengg" --demo --lang en \
    --screenshot "$TMP_DIR/shots" --config "$TMP_DIR/config.json"

mkdir -p "$OUT_DIR"
declare -A MAP=(
    ["01-home.png"]="home.png"
    ["02-engine.png"]="engine.png"
    ["03-apex-3-tkl-tab1.png"]="apex3tkl-illumination.png"
    ["04-rival-3-wireless-gen2-tab1.png"]="rival3-buttons.png"
    ["04-rival-3-wireless-gen2-tab2.png"]="rival3-cpi.png"
)
for src in "${!MAP[@]}"; do
    dst="$OUT_DIR/${MAP[$src]}"
    if command -v pngquant >/dev/null 2>&1; then
        pngquant --quality 85-100 --speed 1 --strip --force --output "$dst" "$TMP_DIR/shots/$src"
    else
        cp -f "$TMP_DIR/shots/$src" "$dst"
    fi
    echo "   $dst ($(stat -c %s "$dst") bytes)"
done
echo ">> Done. Review the images and commit docs/screenshots/."
