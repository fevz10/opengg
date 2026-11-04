#pragma once

#include "devices/Device.h"
#include "models/MouseConfig.h"

#include <QTimer>

class IdleMonitor;

/// SteelSeries Rival 3 Wireless Gen 2 mouse via its 2.4 GHz receiver.
class Rival3WirelessGen2 : public Device
{
    Q_OBJECT
public:
    Rival3WirelessGen2(const HidDeviceInfo &info, HidTransport *transport, QObject *parent = nullptr);

    QString key() const override { return QStringLiteral("rival-3-wireless-gen2"); }
    QString displayName() const override { return QStringLiteral("Rival 3 Wireless Gen 2"); }
    QString modelName() const override { return QStringLiteral("SteelSeries Rival 3 Wireless Gen 2"); }
    Kind kind() const override { return Kind::Mouse; }
    bool hasBattery() const override { return true; }
    bool isWireless() const override { return true; }

    QJsonObject defaultConfig() const override;
    void applyConfig(const QJsonObject &data) override;

    const MouseConfig &mouseConfig() const { return m_mouseConfig; }

    /// Queries the battery level now.
    void requestBattery();
    /// Writes the current settings to the on-board memory now.
    void saveNow();
    /// Re-sends every setting (not only the changed ones) and saves.
    void resendAll();

signals:
    void saved();

protected:
    void onOpened() override;
    void onResponse(int tag, const QByteArray &data) override;
    void onIoError(const QString &error) override;

private:
    enum Tag { TagBattery = 1 };

    void sendDiff(const MouseConfig &previous, const MouseConfig &next, bool force);
    void scheduleSave();

    MouseConfig m_mouseConfig;
    MouseConfig m_lastSent;
    bool m_hasSent = false;
    bool m_resendOnWake = false;
    IdleMonitor *m_idle;
    QTimer m_batteryPoll;
    QTimer m_saveTimer;
    int m_failedQueries = 0;
    int m_ioErrors = 0;
};
