#pragma once

#include "devices/Device.h"
#include "hid/HidTransport.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QTimer>

/**
 * Discovers supported devices (hot-plug aware through periodic enumeration)
 * and owns the Device objects.
 */
class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(HidTransport *transport, QObject *parent = nullptr);
    ~DeviceManager() override;

    void start();
    void stop();
    void rescan();

    QList<Device *> devices() const;
    Device *deviceByKey(const QString &key) const;
    /// SteelSeries HID interfaces that are present but not supported by us.
    QList<HidDeviceInfo> unsupportedDevices() const { return m_unsupported; }
    bool isMock() const { return m_hid->isMock(); }

    /// Describes a known device. Used by the UI for "supported hardware" lists.
    struct SupportedModel {
        quint16 productId;
        int interfaceNumber;
        QString key;
        QString name;
        Device::Kind kind;
    };
    static QList<SupportedModel> supportedModels();

signals:
    void deviceAdded(Device *device);
    void deviceRemoved(Device *device); ///< emitted before the object is deleted
    void devicesChanged();

private:
    Device *createDevice(const HidDeviceInfo &info);

    HidTransport *m_hid;
    QTimer m_timer;
    QHash<QString, Device *> m_devices; // keyed by HID path
    QList<HidDeviceInfo> m_unsupported;
};
