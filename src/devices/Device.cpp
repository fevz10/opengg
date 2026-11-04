#include "devices/Device.h"

#include <QMetaObject>

Device::Device(const HidDeviceInfo &info, HidTransport *transport, QObject *parent)
    : QObject(parent)
    , m_hid(transport)
    , m_info(info)
{
    connect(m_hid, &HidTransport::openResult, this, [this](const QString &path, bool ok, const QString &error) {
        if (path != m_info.path)
            return;
        if (ok) {
            m_opened = true;
            setStatus(Status::Connected);
            onOpened();
        } else {
            setStatus(Status::Error, error);
        }
    });
    connect(m_hid, &HidTransport::readResult, this, [this](const QString &path, int tag, const QByteArray &data) {
        if (path == m_info.path)
            onResponse(tag, data);
    });
    connect(m_hid, &HidTransport::ioError, this, [this](const QString &path, const QString &error) {
        if (path == m_info.path)
            onIoError(error);
    });
}

Device::~Device()
{
    if (m_opened) {
        HidTransport *hid = m_hid;
        const QString path = m_info.path;
        QMetaObject::invokeMethod(hid, [hid, path]() { hid->close(path); }, Qt::QueuedConnection);
    }
}

void Device::open()
{
    HidTransport *hid = m_hid;
    const QString path = m_info.path;
    QMetaObject::invokeMethod(hid, [hid, path]() { hid->open(path); }, Qt::QueuedConnection);
}

void Device::setStatus(Status s, const QString &error)
{
    if (m_status == s && m_error == error)
        return;
    m_status = s;
    m_error = error;
    emit statusChanged();
}

void Device::setBattery(int level, bool charging)
{
    if (m_batteryLevel == level && m_charging == charging)
        return;
    m_batteryLevel = level;
    m_charging = charging;
    emit batteryChanged();
}

void Device::send(const QByteArray &report)
{
    HidTransport *hid = m_hid;
    const QString path = m_info.path;
    const int delay = m_commandDelayMs;
    QMetaObject::invokeMethod(hid, [hid, path, report, delay]() { hid->write(path, report, delay); }, Qt::QueuedConnection);
}

void Device::query(const QByteArray &report, int tag, int readLength, int timeoutMs)
{
    HidTransport *hid = m_hid;
    const QString path = m_info.path;
    const int delay = m_commandDelayMs;
    QMetaObject::invokeMethod(
        hid, [hid, path, report, readLength, timeoutMs, delay, tag]() { hid->writeAndRead(path, report, readLength, timeoutMs, delay, tag); },
        Qt::QueuedConnection);
}
