#include "models/KeyboardConfig.h"

#include <QJsonArray>

KeyboardConfig::KeyboardConfig()
{
    zoneColors.fill(QColor(0xff, 0x52, 0x00));
    shiftColors = {QColor(0xff, 0x52, 0x00), QColor(0xff, 0x00, 0x7a), QColor(0x3d, 0x7b, 0xff), QColor(0x00, 0xd6, 0xa0)};
}

KeyboardConfig KeyboardConfig::defaults()
{
    return KeyboardConfig();
}

QString KeyboardConfig::effectKey(Effect e)
{
    switch (e) {
    case Steady: return QStringLiteral("steady");
    case Breathe: return QStringLiteral("breathe");
    case ColorShift: return QStringLiteral("colorshift");
    case RainbowWave: return QStringLiteral("rainbow");
    case Off: return QStringLiteral("off");
    }
    return QStringLiteral("steady");
}

KeyboardConfig::Effect KeyboardConfig::effectFromKey(const QString &key)
{
    if (key == QLatin1String("breathe")) return Breathe;
    if (key == QLatin1String("colorshift")) return ColorShift;
    if (key == QLatin1String("rainbow")) return RainbowWave;
    if (key == QLatin1String("off")) return Off;
    return Steady;
}

QJsonObject KeyboardConfig::toJson() const
{
    QJsonObject o;
    o.insert(QStringLiteral("effect"), effectKey(effect));
    QJsonArray zones;
    for (const QColor &c : zoneColors)
        zones.append(c.name(QColor::HexRgb));
    o.insert(QStringLiteral("zoneColors"), zones);
    QJsonArray shift;
    for (const QColor &c : shiftColors)
        shift.append(c.name(QColor::HexRgb));
    o.insert(QStringLiteral("shiftColors"), shift);
    o.insert(QStringLiteral("speed"), speed);
    o.insert(QStringLiteral("brightness"), brightness);
    o.insert(QStringLiteral("restoreOnConnect"), restoreOnConnect);
    o.insert(QStringLiteral("followHardwareBrightness"), followHardwareBrightness);
    o.insert(QStringLiteral("idleDimEnabled"), idleDimEnabled);
    o.insert(QStringLiteral("idleMinutes"), idleMinutes);
    return o;
}

KeyboardConfig KeyboardConfig::fromJson(const QJsonObject &o)
{
    KeyboardConfig c;
    c.effect = effectFromKey(o.value(QStringLiteral("effect")).toString(QStringLiteral("steady")));
    const QJsonArray zones = o.value(QStringLiteral("zoneColors")).toArray();
    for (int i = 0; i < ZoneCount && i < zones.size(); ++i) {
        const QColor col(zones.at(i).toString());
        if (col.isValid())
            c.zoneColors[size_t(i)] = col;
    }
    const QJsonArray shift = o.value(QStringLiteral("shiftColors")).toArray();
    if (!shift.isEmpty()) {
        c.shiftColors.clear();
        for (const QJsonValue &v : shift) {
            const QColor col(v.toString());
            if (col.isValid())
                c.shiftColors.append(col);
        }
        if (c.shiftColors.size() < 2)
            c.shiftColors = KeyboardConfig().shiftColors;
    }
    c.speed = qBound(0, o.value(QStringLiteral("speed")).toInt(50), 100);
    c.brightness = qBound(0, o.value(QStringLiteral("brightness")).toInt(100), 100);
    c.restoreOnConnect = o.value(QStringLiteral("restoreOnConnect")).toBool(true);
    c.followHardwareBrightness = o.value(QStringLiteral("followHardwareBrightness")).toBool(true);
    c.idleDimEnabled = o.value(QStringLiteral("idleDimEnabled")).toBool(false);
    c.idleMinutes = qBound(1, o.value(QStringLiteral("idleMinutes")).toInt(10), 120);
    return c;
}

bool KeyboardConfig::operator==(const KeyboardConfig &o) const
{
    return effect == o.effect && zoneColors == o.zoneColors && shiftColors == o.shiftColors && speed == o.speed
        && brightness == o.brightness && restoreOnConnect == o.restoreOnConnect
        && followHardwareBrightness == o.followHardwareBrightness && idleDimEnabled == o.idleDimEnabled
        && idleMinutes == o.idleMinutes;
}
