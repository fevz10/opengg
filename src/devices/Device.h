#pragma once

#include "hid/HidTransport.h"

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QString>

/**
 * Base class of a configurable SteelSeries device. Lives in the GUI thread and
 * talks to the hardware through the HidTransport worker.
 */
class Device : public QObject
{
    Q_OBJECT
public:
    enum class Kind { Keyboard, Mouse };
    enum class Status {
        Connecting, ///< interface found, opening
        Connected,  ///< device answers
        Asleep,     ///< receiver present but the wireless device does not answer
        Error       ///< could not be opened (permissions, busy, ...)
    };

    Device(const HidDeviceInfo &info, HidTransport *transport, QObject *parent = nullptr);
    ~Device() override;

    const HidDeviceInfo &info() const { return m_info; }
    QString path() const { return m_info.path; }

    /// Stable identifier used for persistence, e.g. "apex-3-tkl".
    virtual QString key() const = 0;
    /// Short marketing name, e.g. "Apex 3 TKL".
    virtual QString displayName() const = 0;
    /// Full product name, e.g. "SteelSeries Apex 3 TKL".
    virtual QString modelName() const = 0;
    virtual Kind kind() const = 0;
    virtual bool hasBattery() const { return false; }
    virtual bool isWireless() const { return false; }

    /// Default config data for a brand new "config" entry.
    virtual QJsonObject defaultConfig() const = 0;
    /// Applies (and remembers) the given config; sends it to the hardware.
    virtual void applyConfig(const QJsonObject &data) = 0;
    /// Last config handed to applyConfig().
    const QJsonObject &currentConfig() const { return m_config; }

    Status status() const { return m_status; }
    QString errorString() const { return m_error; }
    bool isConnected() const { return m_status == Status::Connected; }
    int batteryLevel() const { return m_batteryLevel; }
    bool isCharging() const { return m_charging; }

    /// Opens the HID interface (asynchronously).
    void open();

signals:
    void statusChanged();
    void batteryChanged();
    void configApplied();

protected:
    void setStatus(Status s, const QString &error = QString());
    void setBattery(int level, bool charging);
    void send(const QByteArray &report);
    void query(const QByteArray &report, int tag, int readLength = 64, int timeoutMs = 200);

    /// Called once the interface is open.
    virtual void onOpened() = 0;
    virtual void onResponse(int tag, const QByteArray &data) { Q_UNUSED(tag); Q_UNUSED(data); }
    virtual void onIoError(const QString &error) { Q_UNUSED(error); }

    HidTransport *m_hid;
    HidDeviceInfo m_info;
    QJsonObject m_config;
    int m_commandDelayMs = 0;
    bool m_opened = false;

private:
    Status m_status = Status::Connecting;
    QString m_error;
    int m_batteryLevel = -1;
    bool m_charging = false;
};
