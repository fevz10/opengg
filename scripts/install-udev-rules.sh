#!/usr/bin/env bash
# Installs the udev rules needed to access the devices without root.
# Normally done automatically by the .deb package; use this when running
# the application from a build directory.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RULES_SRC="$ROOT_DIR/packaging/70-opengg.rules"
RULES_DST="/etc/udev/rules.d/70-opengg.rules"

if [[ $EUID -ne 0 ]]; then
    exec sudo "$0" "$@"
fi

install -m 0644 "$RULES_SRC" "$RULES_DST"
udevadm control --reload-rules
udevadm trigger --subsystem-match=hidraw --action=add
udevadm trigger --subsystem-match=usb --attr-match=idVendor=1038 --action=add
echo "udev rules installed to $RULES_DST"
echo "If a device is still not accessible, unplug and replug it (or log out and back in)."
