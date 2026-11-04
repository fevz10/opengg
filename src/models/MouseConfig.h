#pragma once

#include "devices/Protocol.h"

#include <QJsonObject>
#include <QList>
#include <QString>

#include <array>

/// Settings of the Rival 3 Wireless Gen 2 ("config" in GG terminology).
struct MouseConfig {
    static constexpr int PresetCount = 5;
    static constexpr int ButtonCount = 6;

    struct Preset {
        bool enabled = false;
        int x = 800;
        int y = 800;
        bool operator==(const Preset &o) const { return enabled == o.enabled && x == o.x && y == o.y; }
    };

    std::array<Preset, PresetCount> presets{};
    int activePreset = 0; ///< index into presets (must be enabled)
    bool separateXY = false;
    int pollingRate = 1000;
    std::array<Protocol::ButtonAction, ButtonCount> buttons{};
    bool lowBatteryWarning = true;

    MouseConfig();

    /// Enabled presets in order, as the device expects them, and the
    /// position of the active preset in that list.
    QList<Protocol::Rival3WirelessGen2::DpiPreset> enabledPresets(int *activeIndexOut) const;
    int enabledCount() const;
    /// Keeps the structure valid: at least one enabled preset, active one enabled.
    void normalize();

    QJsonObject toJson() const;
    static MouseConfig fromJson(const QJsonObject &obj);
    static MouseConfig defaults();

    static QJsonObject actionToJson(const Protocol::ButtonAction &a);
    static Protocol::ButtonAction actionFromJson(const QJsonObject &o, const Protocol::ButtonAction &fallback);

    bool operator==(const MouseConfig &o) const;
    bool operator!=(const MouseConfig &o) const { return !(*this == o); }
};
