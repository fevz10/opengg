#pragma once

#include "ui/AppContext.h"

#include <QHash>
#include <QObject>

class Device;

/**
 * Glue between the device manager and the config store: applies the active
 * config when a device appears or when the config changes, and raises user
 * notifications (connect/disconnect, low battery, permission problems).
 */
class DeviceController : public QObject
{
    Q_OBJECT
public:
    explicit DeviceController(const AppContext &ctx, QObject *parent = nullptr);

signals:
    void permissionProblem(Device *device);

private:
    void onDeviceAdded(Device *device);
    void onDeviceRemoved(Device *device);
    void applyActive(Device *device);

    AppContext m_ctx;
    QHash<Device *, bool> m_lowBatteryWarned;
    QHash<Device *, bool> m_announced;
};
