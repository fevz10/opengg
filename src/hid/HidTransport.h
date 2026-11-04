#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

struct hid_device_;

/// Description of one HID interface as returned by hidapi enumeration.
struct HidDeviceInfo {
    QString path;
    quint16 vendorId = 0;
    quint16 productId = 0;
    QString manufacturer;
    QString product;
    QString serial;
    int interfaceNumber = -1;
    quint16 usagePage = 0;
    quint16 usage = 0;
};

/**
 * Thin asynchronous wrapper around hidapi (hidraw backend).
 *
 * The object is meant to live in its own QThread: every slot is invoked
 * through the Qt event loop so the GUI thread never blocks on USB I/O (some
 * devices need a small delay between commands and reads use timeouts).
 *
 * In "mock" mode no hardware is touched; writes are logged and reads return
 * plausible answers. This powers the `--demo` command line switch.
 */
class HidTransport : public QObject
{
    Q_OBJECT
public:
    explicit HidTransport(bool mock = false, QObject *parent = nullptr);
    ~HidTransport() override;

    bool isMock() const { return m_mock; }

    /// Enumerates HID interfaces of the given vendor. Thread safe.
    static QList<HidDeviceInfo> enumerate(quint16 vendorId, bool mock);

public slots:
    void open(const QString &path);
    void close(const QString &path);
    void write(const QString &path, const QByteArray &report, int postDelayMs);
    void writeAndRead(const QString &path, const QByteArray &report, int readLength, int timeoutMs, int postDelayMs, int tag);

signals:
    void openResult(const QString &path, bool ok, const QString &error);
    void readResult(const QString &path, int tag, const QByteArray &data);
    void ioError(const QString &path, const QString &error);
    void logLine(const QString &line);

private:
    hid_device_ *handle(const QString &path);
    QByteArray mockResponse(const QByteArray &report) const;
    static QString lastError(hid_device_ *dev);

    bool m_mock;
    quint8 m_mockBrightness = 0x10;
    QHash<QString, hid_device_ *> m_open;
};
