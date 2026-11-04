#include "ui/DeviceController.h"

#include "core/ConfigStore.h"
#include "devices/Apex3Tkl.h"
#include "devices/Device.h"
#include "devices/DeviceManager.h"
#include "devices/Rival3WirelessGen2.h"
#include "models/KeyboardConfig.h"
#include "models/MouseConfig.h"
#include "ui/widgets/Toast.h"

DeviceController::DeviceController(const AppContext &ctx, QObject *parent)
    : QObject(parent)
    , m_ctx(ctx)
{
    connect(m_ctx.devices, &DeviceManager::deviceAdded, this, &DeviceController::onDeviceAdded);
    connect(m_ctx.devices, &DeviceManager::deviceRemoved, this, &DeviceController::onDeviceRemoved);

    connect(m_ctx.store, &ConfigStore::configDataChanged, this, [this](const QString &key, const QString &id) {
        if (Device *d = m_ctx.devices->deviceByKey(key); d && m_ctx.store->activeConfigId(key) == id)
            d->applyConfig(m_ctx.store->config(key, id).data);
    });
    connect(m_ctx.store, &ConfigStore::activeConfigChanged, this, [this](const QString &key, const QString &) {
        if (Device *d = m_ctx.devices->deviceByKey(key))
            applyActive(d);
    });

    for (Device *d : m_ctx.devices->devices())
        onDeviceAdded(d);
}

void DeviceController::applyActive(Device *device)
{
    device->applyConfig(m_ctx.store->activeConfig(device->key()).data);
}

void DeviceController::onDeviceAdded(Device *device)
{
    const QString key = device->key();
    m_ctx.store->ensureDefaultConfig(key, device->defaultConfig(), tr("Default config"));
    applyActive(device);

    connect(device, &Device::statusChanged, this, [this, device]() {
        const bool notify = m_ctx.store->value(QStringLiteral("notifications"), true).toBool();
        switch (device->status()) {
        case Device::Status::Connected:
            if (!m_announced.value(device, false)) {
                m_announced.insert(device, true);
                if (notify && m_ctx.toasts)
                    m_ctx.toasts->show(tr("%1 connected").arg(device->modelName()), Toast::Success);
            }
            break;
        case Device::Status::Error:
            if (m_ctx.toasts)
                m_ctx.toasts->show(tr("%1 could not be opened: %2").arg(device->modelName(), device->errorString()), Toast::Error, 6000);
            emit permissionProblem(device);
            break;
        default:
            break;
        }
    });

    connect(device, &Device::batteryChanged, this, [this, device]() {
        const int lvl = device->batteryLevel();
        if (lvl < 0)
            return;
        bool warnEnabled = true;
        if (auto *rival = qobject_cast<Rival3WirelessGen2 *>(device))
            warnEnabled = rival->mouseConfig().lowBatteryWarning;
        if (lvl <= 15 && !device->isCharging() && warnEnabled && !m_lowBatteryWarned.value(device, false)) {
            m_lowBatteryWarned.insert(device, true);
            if (m_ctx.toasts)
                m_ctx.toasts->show(tr("%1 battery low (%2%)").arg(device->displayName()).arg(lvl), Toast::Warning, 6000);
        } else if (lvl > 25) {
            m_lowBatteryWarned.insert(device, false);
        }
    });

    if (auto *apex = qobject_cast<Apex3Tkl *>(device)) {
        connect(apex, &Apex3Tkl::hardwareBrightnessChanged, this, [this, apex](int percent) {
            const QString key = apex->key();
            const ConfigStore::NamedConfig active = m_ctx.store->activeConfig(key);
            KeyboardConfig cfg = KeyboardConfig::fromJson(active.data);
            if (cfg.brightness == percent)
                return;
            cfg.brightness = percent;
            m_ctx.store->updateConfigData(key, active.id, cfg.toJson());
        });
    }
}

void DeviceController::onDeviceRemoved(Device *device)
{
    const bool notify = m_ctx.store->value(QStringLiteral("notifications"), true).toBool();
    if (notify && m_ctx.toasts && m_announced.value(device, false))
        m_ctx.toasts->show(tr("%1 disconnected").arg(device->modelName()), Toast::Info);
    m_announced.remove(device);
    m_lowBatteryWarned.remove(device);
}
