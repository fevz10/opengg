#include "devices/DeviceManager.h"

#include "devices/Apex3Tkl.h"
#include "devices/Protocol.h"
#include "devices/Rival3WirelessGen2.h"

#include <QLoggingCategory>
#include <QSet>

Q_LOGGING_CATEGORY(lcDevices, "ssgg.devices")

namespace {
constexpr int kScanIntervalMs = 2000;
}

DeviceManager::DeviceManager(HidTransport *transport, QObject *parent)
    : QObject(parent)
    , m_hid(transport)
{
    m_timer.setInterval(kScanIntervalMs);
    connect(&m_timer, &QTimer::timeout, this, &DeviceManager::rescan);
}

DeviceManager::~DeviceManager()
{
    stop();
    const auto list = m_devices.values();
    m_devices.clear();
    qDeleteAll(list);
}

QList<DeviceManager::SupportedModel> DeviceManager::supportedModels()
{
    return {
        {Protocol::Apex3Tkl::ProductId, Protocol::Apex3Tkl::InterfaceNumber, QStringLiteral("apex-3-tkl"),
         QStringLiteral("SteelSeries Apex 3 TKL"), Device::Kind::Keyboard},
        {Protocol::Rival3WirelessGen2::ProductId, Protocol::Rival3WirelessGen2::InterfaceNumber,
         QStringLiteral("rival-3-wireless-gen2"), QStringLiteral("SteelSeries Rival 3 Wireless Gen 2"), Device::Kind::Mouse},
    };
}

void DeviceManager::start()
{
    rescan();
    m_timer.start();
}

void DeviceManager::stop()
{
    m_timer.stop();
}

QList<Device *> DeviceManager::devices() const
{
    QList<Device *> list = m_devices.values();
    std::sort(list.begin(), list.end(), [](Device *a, Device *b) {
        if (a->kind() != b->kind())
            return a->kind() == Device::Kind::Keyboard; // keyboards first
        return a->displayName() < b->displayName();
    });
    return list;
}

Device *DeviceManager::deviceByKey(const QString &key) const
{
    for (Device *d : m_devices)
        if (d->key() == key)
            return d;
    return nullptr;
}

Device *DeviceManager::createDevice(const HidDeviceInfo &info)
{
    if (info.productId == Protocol::Apex3Tkl::ProductId && info.interfaceNumber == Protocol::Apex3Tkl::InterfaceNumber)
        return new Apex3Tkl(info, m_hid, this);
    if (info.productId == Protocol::Rival3WirelessGen2::ProductId
        && info.interfaceNumber == Protocol::Rival3WirelessGen2::InterfaceNumber)
        return new Rival3WirelessGen2(info, m_hid, this);
    return nullptr;
}

void DeviceManager::rescan()
{
    const QList<HidDeviceInfo> found = HidTransport::enumerate(Protocol::SteelSeriesVendorId, m_hid->isMock());

    QSet<QString> seenPaths;
    QList<HidDeviceInfo> unsupported;
    QSet<quint16> supportedPids;
    for (const SupportedModel &m : supportedModels())
        supportedPids.insert(m.productId);

    bool changed = false;
    for (const HidDeviceInfo &info : found) {
        if (!supportedPids.contains(info.productId)) {
            // Report each unsupported product once (not every interface).
            bool dup = false;
            for (const HidDeviceInfo &u : unsupported)
                dup = dup || u.productId == info.productId;
            if (!dup)
                unsupported.append(info);
            continue;
        }
        if (m_devices.contains(info.path)) {
            seenPaths.insert(info.path);
            continue;
        }
        Device *dev = createDevice(info);
        if (!dev)
            continue; // another interface of a supported device
        seenPaths.insert(info.path);
        m_devices.insert(info.path, dev);
        qCInfo(lcDevices) << "Device found:" << dev->modelName() << info.path;
        dev->open();
        emit deviceAdded(dev);
        changed = true;
    }

    // Removed devices
    const QList<QString> paths = m_devices.keys();
    for (const QString &path : paths) {
        if (seenPaths.contains(path))
            continue;
        Device *dev = m_devices.take(path);
        qCInfo(lcDevices) << "Device removed:" << dev->modelName() << path;
        emit deviceRemoved(dev);
        dev->deleteLater();
        changed = true;
    }

    if (unsupported.size() != m_unsupported.size()) {
        changed = true;
    } else {
        for (int i = 0; i < unsupported.size(); ++i)
            changed = changed || unsupported.at(i).productId != m_unsupported.at(i).productId;
    }
    m_unsupported = unsupported;

    if (changed)
        emit devicesChanged();
}
