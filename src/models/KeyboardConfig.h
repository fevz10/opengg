#pragma once

#include <QColor>
#include <QJsonObject>
#include <QList>
#include <QString>

#include <array>

/// Illumination settings of the Apex 3 TKL ("config" in GG terminology).
struct KeyboardConfig {
    enum Effect {
        Steady = 0,
        Breathe,
        ColorShift,
        RainbowWave, ///< hardware effect
        Off
    };

    static constexpr int ZoneCount = 8;

    Effect effect = Steady;
    std::array<QColor, ZoneCount> zoneColors{};
    QList<QColor> shiftColors; ///< gradient stops used by ColorShift
    int speed = 50;            ///< 0..100
    int brightness = 100;      ///< 0..100

    // Behaviour (software side)
    bool restoreOnConnect = true;
    bool followHardwareBrightness = true;
    bool idleDimEnabled = false;
    int idleMinutes = 10;

    KeyboardConfig();

    QJsonObject toJson() const;
    static KeyboardConfig fromJson(const QJsonObject &obj);
    static KeyboardConfig defaults();

    static QString effectKey(Effect e);
    static Effect effectFromKey(const QString &key);

    bool operator==(const KeyboardConfig &o) const;
    bool operator!=(const KeyboardConfig &o) const { return !(*this == o); }
};
