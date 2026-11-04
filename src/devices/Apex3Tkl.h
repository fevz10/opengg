#pragma once

#include "devices/Device.h"
#include "lighting/LightingEngine.h"
#include "models/KeyboardConfig.h"

#include <QTimer>

class IdleMonitor;

/// SteelSeries Apex 3 TKL keyboard (8 zone RGB).
class Apex3Tkl : public Device
{
    Q_OBJECT
public:
    Apex3Tkl(const HidDeviceInfo &info, HidTransport *transport, QObject *parent = nullptr);

    QString key() const override { return QStringLiteral("apex-3-tkl"); }
    QString displayName() const override { return QStringLiteral("Apex 3 TKL"); }
    QString modelName() const override { return QStringLiteral("SteelSeries Apex 3 TKL"); }
    Kind kind() const override { return Kind::Keyboard; }

    QJsonObject defaultConfig() const override;
    void applyConfig(const QJsonObject &data) override;

    const KeyboardConfig &keyboardConfig() const { return m_keyboardConfig; }
    LightingEngine *engine() { return m_engine; }
    IdleMonitor *idleMonitor() { return m_idle; }

    /// Forces a brightness readback right now.
    void requestBrightness();

signals:
    /// Emitted when the brightness was changed on the keyboard itself
    /// (Fn+F11 / Fn+F12); `percent` is already converted to 0..100.
    void hardwareBrightnessChanged(int percent);

protected:
    void onOpened() override;
    void onResponse(int tag, const QByteArray &data) override;
    void onIoError(const QString &error) override;

private:
    enum Tag { TagBrightness = 1 };

    void updateMonitors();

    KeyboardConfig m_keyboardConfig;
    LightingEngine *m_engine;
    IdleMonitor *m_idle;
    QTimer m_brightnessPoll;
    int m_expectedLevel = -1;
    int m_ioErrors = 0;
};
