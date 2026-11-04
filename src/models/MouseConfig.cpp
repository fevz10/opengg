#include "models/MouseConfig.h"

#include <QJsonArray>

using Protocol::ButtonAction;

MouseConfig::MouseConfig()
{
    presets = {{{true, 800, 800}, {true, 1600, 1600}, {false, 3200, 3200}, {false, 6400, 6400}, {false, 12000, 12000}}};
    buttons = Protocol::Rival3WirelessGen2::defaultButtons();
}

MouseConfig MouseConfig::defaults()
{
    return MouseConfig();
}

QList<Protocol::Rival3WirelessGen2::DpiPreset> MouseConfig::enabledPresets(int *activeIndexOut) const
{
    QList<Protocol::Rival3WirelessGen2::DpiPreset> list;
    int active = 0;
    for (int i = 0; i < PresetCount; ++i) {
        const Preset &p = presets[size_t(i)];
        if (!p.enabled)
            continue;
        if (i == activePreset)
            active = list.size();
        list.append({p.x, separateXY ? p.y : p.x});
    }
    if (activeIndexOut)
        *activeIndexOut = active;
    return list;
}

int MouseConfig::enabledCount() const
{
    int n = 0;
    for (const Preset &p : presets)
        n += p.enabled ? 1 : 0;
    return n;
}

void MouseConfig::normalize()
{
    if (enabledCount() == 0)
        presets[0].enabled = true;
    activePreset = qBound(0, activePreset, PresetCount - 1);
    if (!presets[size_t(activePreset)].enabled) {
        for (int i = 0; i < PresetCount; ++i) {
            if (presets[size_t(i)].enabled) {
                activePreset = i;
                break;
            }
        }
    }
    for (Preset &p : presets) {
        p.x = Protocol::Rival3WirelessGen2::nearestDpi(p.x);
        p.y = Protocol::Rival3WirelessGen2::nearestDpi(p.y);
    }
    if (pollingRate != 125 && pollingRate != 250 && pollingRate != 500 && pollingRate != 1000)
        pollingRate = 1000;
}

QJsonObject MouseConfig::actionToJson(const ButtonAction &a)
{
    static const char *names[] = {"mouse", "disabled", "cpi", "scrollup", "scrolldown", "keyboard", "multimedia"};
    QJsonObject o;
    o.insert(QStringLiteral("type"), QString::fromLatin1(names[int(a.type)]));
    o.insert(QStringLiteral("code"), a.code);
    return o;
}

ButtonAction MouseConfig::actionFromJson(const QJsonObject &o, const ButtonAction &fallback)
{
    if (o.isEmpty())
        return fallback;
    const QString t = o.value(QStringLiteral("type")).toString();
    ButtonAction a;
    a.code = o.value(QStringLiteral("code")).toInt(0);
    if (t == QLatin1String("mouse")) a.type = ButtonAction::MouseButton;
    else if (t == QLatin1String("disabled")) a.type = ButtonAction::Disabled;
    else if (t == QLatin1String("cpi")) a.type = ButtonAction::CpiSwitch;
    else if (t == QLatin1String("scrollup")) a.type = ButtonAction::ScrollUp;
    else if (t == QLatin1String("scrolldown")) a.type = ButtonAction::ScrollDown;
    else if (t == QLatin1String("keyboard")) a.type = ButtonAction::Keyboard;
    else if (t == QLatin1String("multimedia")) a.type = ButtonAction::Multimedia;
    else return fallback;
    return a;
}

QJsonObject MouseConfig::toJson() const
{
    QJsonObject o;
    QJsonArray pr;
    for (const Preset &p : presets) {
        pr.append(QJsonObject{{QStringLiteral("enabled"), p.enabled}, {QStringLiteral("x"), p.x}, {QStringLiteral("y"), p.y}});
    }
    o.insert(QStringLiteral("presets"), pr);
    o.insert(QStringLiteral("activePreset"), activePreset);
    o.insert(QStringLiteral("separateXY"), separateXY);
    o.insert(QStringLiteral("pollingRate"), pollingRate);
    QJsonArray btns;
    for (const ButtonAction &a : buttons)
        btns.append(actionToJson(a));
    o.insert(QStringLiteral("buttons"), btns);
    o.insert(QStringLiteral("lowBatteryWarning"), lowBatteryWarning);
    return o;
}

MouseConfig MouseConfig::fromJson(const QJsonObject &o)
{
    MouseConfig c;
    const QJsonArray pr = o.value(QStringLiteral("presets")).toArray();
    for (int i = 0; i < PresetCount && i < pr.size(); ++i) {
        const QJsonObject p = pr.at(i).toObject();
        c.presets[size_t(i)].enabled = p.value(QStringLiteral("enabled")).toBool(c.presets[size_t(i)].enabled);
        c.presets[size_t(i)].x = p.value(QStringLiteral("x")).toInt(c.presets[size_t(i)].x);
        c.presets[size_t(i)].y = p.value(QStringLiteral("y")).toInt(c.presets[size_t(i)].y);
    }
    c.activePreset = o.value(QStringLiteral("activePreset")).toInt(0);
    c.separateXY = o.value(QStringLiteral("separateXY")).toBool(false);
    c.pollingRate = o.value(QStringLiteral("pollingRate")).toInt(1000);
    const QJsonArray btns = o.value(QStringLiteral("buttons")).toArray();
    const auto defaults = Protocol::Rival3WirelessGen2::defaultButtons();
    for (int i = 0; i < ButtonCount; ++i) {
        c.buttons[size_t(i)] = i < btns.size() ? actionFromJson(btns.at(i).toObject(), defaults[size_t(i)]) : defaults[size_t(i)];
    }
    c.lowBatteryWarning = o.value(QStringLiteral("lowBatteryWarning")).toBool(true);
    c.normalize();
    return c;
}

bool MouseConfig::operator==(const MouseConfig &o) const
{
    return presets == o.presets && activePreset == o.activePreset && separateXY == o.separateXY
        && pollingRate == o.pollingRate && buttons == o.buttons && lowBatteryWarning == o.lowBatteryWarning;
}
