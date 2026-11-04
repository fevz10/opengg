#!/usr/bin/env bash
# Builds the .deb package for OpenGG.
#
# Usage:
#   ./scripts/build-deb.sh            # configure + build + package into ./dist
#   ./scripts/build-deb.sh --deps     # also install build dependencies with apt
#   ./scripts/build-deb.sh --install  # install the produced .deb with apt afterwards
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build}"
DIST_DIR="${DIST_DIR:-$ROOT_DIR/dist}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 2)}"

INSTALL_DEPS=0
INSTALL_PKG=0
for arg in "$@"; do
    case "$arg" in
        --deps) INSTALL_DEPS=1 ;;
        --install) INSTALL_PKG=1 ;;
        -h|--help)
            sed -n '2,8p' "$0"; exit 0 ;;
        *) echo "Unknown option: $arg" >&2; exit 2 ;;
    esac
done

install_deps() {
    echo ">> Installing build dependencies (apt)"
    local pkgs=(build-essential cmake pkg-config dpkg-dev file
                qt6-base-dev qt6-base-dev-tools qt6-l10n-tools qt6-tools-dev-tools
                libhidapi-dev libgl1-mesa-dev)
    # Qt SVG dev package is named differently across releases
    if apt-cache show qt6-svg-dev >/dev/null 2>&1; then
        pkgs+=(qt6-svg-dev)
    else
        pkgs+=(libqt6svg6-dev)
    fi
    sudo apt-get update
    sudo apt-get install -y "${pkgs[@]}"
}

if [[ $INSTALL_DEPS -eq 1 ]]; then
    install_deps
fi

echo ">> Configuring ($BUILD_DIR)"
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DSSGG_BUILD_TESTS=ON

echo ">> Building"
cmake --build "$BUILD_DIR" --parallel "$JOBS"

if [[ -x "$BUILD_DIR/protocol_tests" ]]; then
    echo ">> Running protocol tests"
    (cd "$BUILD_DIR" && QT_QPA_PLATFORM=offscreen ctest --output-on-failure)
fi

echo ">> Packaging (.deb)"
(cd "$BUILD_DIR" && cpack -G DEB)

mkdir -p "$DIST_DIR"
cp -f "$BUILD_DIR"/*.deb "$DIST_DIR"/
echo
echo "Package(s) written to $DIST_DIR:"
ls -1 "$DIST_DIR"/*.deb

if [[ $INSTALL_PKG -eq 1 ]]; then
    DEB="$(ls -1t "$DIST_DIR"/*.deb | head -n1)"
    echo ">> Installing $DEB"
    sudo apt-get install -y "$DEB"
fi
