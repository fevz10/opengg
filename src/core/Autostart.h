#pragma once

#include <QString>

/// Manages the XDG autostart entry (~/.config/autostart/opengg.desktop).
namespace Autostart {

QString entryPath();
bool isEnabled();
bool setEnabled(bool enabled, bool startMinimized);
/// Replaces an autostart entry left behind by the former "steelseries-gg" name.
bool migrateLegacyEntry(bool startMinimized);

} // namespace Autostart
