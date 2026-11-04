#include "hid/HidTransport.h"

#include <QDateTime>
#include <QLoggingCategory>
#include <QThread>

#include <hidapi.h>

Q_LOGGING_CATEGORY(lcHid, "ssgg.hid")

namespace {

QString hexDump(const QByteArray &data, int maxBytes = 40)
{
    QString s;
    const int n = qMin(data.size(), maxBytes);
    for (int i = 0; i < n; ++i) {
        s += QStringLiteral("%1 ").arg(quint8(data.at(i)), 2, 16, QLatin1Char('0'));
    }
    if (data.size() > maxBytes)
        s += QStringLiteral("… (%1 bytes)").arg(data.size());
    return s.trimmed().toUpper();
}

struct HidLibrary {
    HidLibrary() { hid_init(); }
    ~HidLibrary() { hid_exit(); }
};

void ensureHidInit()
{
    static HidLibrary lib;
    (void)lib;
}

} // namespace

HidTransport::HidTransport(bool mock, QObject *parent)
    : QObject(parent)
    , m_mock(mock)
{
    if (!m_mock)
        ensureHidInit();
    connect(this, &HidTransport::logLine, this, [](const QString &line) { qCDebug(lcHid).noquote() << line; });
}

HidTransport::~HidTransport()
{
    for (auto it = m_open.begin(); it != m_open.end(); ++it) {
        if (it.value())
            hid_close(it.value());
    }
    m_open.clear();
}

QList<HidDeviceInfo> HidTransport::enumerate(quint16 vendorId, bool mock)
{
    QList<HidDeviceInfo> out;
    if (mock) {
        HidDeviceInfo kb;
        kb.path = QStringLiteral("mock:1038:1622:01");
        kb.vendorId = vendorId;
        kb.productId = 0x1622;
        kb.manufacturer = QStringLiteral("SteelSeries");
        kb.product = QStringLiteral("SteelSeries Apex 3 TKL");
        kb.interfaceNumber = 1;
        kb.usagePage = 0xFFC0;
        kb.usage = 1;
        out.append(kb);

        HidDeviceInfo mouse;
        mouse.path = QStringLiteral("mock:1038:1872:03");
        mouse.vendorId = vendorId;
        mouse.productId = 0x1872;
        mouse.manufacturer = QStringLiteral("SteelSeries");
        mouse.product = QStringLiteral("SteelSeries Rival 3 Wireless Gen 2");
        mouse.interfaceNumber = 3;
        out.append(mouse);
        return out;
    }

    ensureHidInit();
    hid_device_info *list = hid_enumerate(vendorId, 0x0);
    for (hid_device_info *cur = list; cur; cur = cur->next) {
        HidDeviceInfo info;
        info.path = QString::fromLocal8Bit(cur->path);
        info.vendorId = cur->vendor_id;
        info.productId = cur->product_id;
        info.manufacturer = cur->manufacturer_string ? QString::fromWCharArray(cur->manufacturer_string) : QString();
        info.product = cur->product_string ? QString::fromWCharArray(cur->product_string) : QString();
        info.serial = cur->serial_number ? QString::fromWCharArray(cur->serial_number) : QString();
        info.interfaceNumber = cur->interface_number;
        info.usagePage = cur->usage_page;
        info.usage = cur->usage;
        out.append(info);
    }
    hid_free_enumeration(list);
    return out;
}

QString HidTransport::lastError(hid_device_ *dev)
{
    const wchar_t *err = hid_error(dev);
    return err ? QString::fromWCharArray(err) : QStringLiteral("unknown error");
}

hid_device_ *HidTransport::handle(const QString &path)
{
    auto it = m_open.find(path);
    return it == m_open.end() ? nullptr : it.value();
}

void HidTransport::open(const QString &path)
{
    if (m_mock) {
        m_open.insert(path, nullptr);
        emit logLine(QStringLiteral("[mock] open %1").arg(path));
        emit openResult(path, true, QString());
        return;
    }
    if (m_open.contains(path)) {
        emit openResult(path, true, QString());
        return;
    }
    hid_device *dev = hid_open_path(path.toLocal8Bit().constData());
    if (!dev) {
        const QString err = lastError(nullptr);
        qCWarning(lcHid) << "hid_open_path failed for" << path << err;
        emit logLine(QStringLiteral("open %1 failed: %2").arg(path, err));
        emit openResult(path, false, err);
        return;
    }
    m_open.insert(path, dev);
    emit logLine(QStringLiteral("open %1 ok").arg(path));
    emit openResult(path, true, QString());
}

void HidTransport::close(const QString &path)
{
    auto it = m_open.find(path);
    if (it == m_open.end())
        return;
    if (it.value())
        hid_close(it.value());
    m_open.erase(it);
    emit logLine(QStringLiteral("close %1").arg(path));
}

void HidTransport::write(const QString &path, const QByteArray &report, int postDelayMs)
{
    if (m_mock) {
        if (!m_open.contains(path))
            m_open.insert(path, nullptr);
        if (report.size() >= 3 && quint8(report.at(1)) == 0x23)
            m_mockBrightness = quint8(report.at(2));
        emit logLine(QStringLiteral("[mock] write %1 ← %2").arg(path.section(QLatin1Char(':'), 2, 2), hexDump(report)));
        if (postDelayMs > 0)
            QThread::msleep(uint(qMin(postDelayMs, 5)));
        return;
    }
    hid_device_ *dev = handle(path);
    if (!dev) {
        emit ioError(path, QStringLiteral("device not open"));
        return;
    }
    const int written = hid_write(dev, reinterpret_cast<const unsigned char *>(report.constData()), size_t(report.size()));
    if (written < 0) {
        const QString err = lastError(dev);
        qCWarning(lcHid) << "hid_write failed" << path << err;
        emit logLine(QStringLiteral("write %1 failed: %2").arg(path, err));
        emit ioError(path, err);
        return;
    }
    emit logLine(QStringLiteral("write %1 ← %2").arg(path.section(QLatin1Char('/'), -1), hexDump(report)));
    if (postDelayMs > 0)
        QThread::msleep(uint(postDelayMs));
}

QByteArray HidTransport::mockResponse(const QByteArray &report) const
{
    if (report.size() < 2)
        return {};
    const quint8 cmd = quint8(report.at(1));
    switch (cmd) {
    case 0xAA: { // battery level query: level, ?, charging
        QByteArray r(3, '\0');
        r[0] = char(73);
        r[2] = char(0);
        return r;
    }
    case 0xA3: { // keyboard brightness query
        QByteArray r(2, '\0');
        r[0] = char(0xA3);
        r[1] = char(m_mockBrightness);
        return r;
    }
    default:
        return {};
    }
}

void HidTransport::writeAndRead(const QString &path, const QByteArray &report, int readLength, int timeoutMs, int postDelayMs, int tag)
{
    if (m_mock) {
        emit logLine(QStringLiteral("[mock] query %1 ← %2").arg(path.section(QLatin1Char(':'), 2, 2), hexDump(report)));
        emit readResult(path, tag, mockResponse(report));
        return;
    }
    hid_device_ *dev = handle(path);
    if (!dev) {
        emit ioError(path, QStringLiteral("device not open"));
        emit readResult(path, tag, QByteArray());
        return;
    }

    // Drain stale input reports so the answer we read belongs to our query.
    {
        unsigned char junk[64];
        hid_set_nonblocking(dev, 1);
        while (hid_read(dev, junk, sizeof junk) > 0) {
        }
        hid_set_nonblocking(dev, 0);
    }

    const int written = hid_write(dev, reinterpret_cast<const unsigned char *>(report.constData()), size_t(report.size()));
    if (written < 0) {
        const QString err = lastError(dev);
        emit logLine(QStringLiteral("query %1 failed: %2").arg(path, err));
        emit ioError(path, err);
        emit readResult(path, tag, QByteArray());
        return;
    }

    QByteArray buf(qMax(readLength, 64), '\0');
    const int n = hid_read_timeout(dev, reinterpret_cast<unsigned char *>(buf.data()), size_t(buf.size()), timeoutMs);
    if (n <= 0) {
        emit logLine(QStringLiteral("query %1 ← %2 : no answer").arg(path.section(QLatin1Char('/'), -1), hexDump(report)));
        emit readResult(path, tag, QByteArray());
    } else {
        buf.truncate(n);
        emit logLine(QStringLiteral("query %1 ← %2 → %3").arg(path.section(QLatin1Char('/'), -1), hexDump(report), hexDump(buf, 16)));
        emit readResult(path, tag, buf);
    }
    if (postDelayMs > 0)
        QThread::msleep(uint(postDelayMs));
}
