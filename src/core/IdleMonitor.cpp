#include "core/IdleMonitor.h"

#if defined(SSGG_HAVE_DBUS)
#include <QDBusConnection>
#include <QDBusMessage>
#endif

IdleMonitor::IdleMonitor(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(5000);
    connect(&m_timer, &QTimer::timeout, this, &IdleMonitor::poll);
}

void IdleMonitor::setEnabled(bool enabled)
{
    if (enabled) {
        if (!m_timer.isActive()) {
            m_timer.start();
            poll();
        }
    } else {
        m_timer.stop();
        if (m_idle) {
            m_idle = false;
            emit idleChanged(false);
        }
    }
}

qint64 IdleMonitor::queryIdleMs()
{
#if defined(SSGG_HAVE_DBUS)
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return -1;

    // Plain method calls (no introspection, short timeout) so that polling is
    // cheap enough to run every few seconds on the GUI thread.
    auto call = [&bus](const char *service, const char *path, const char *iface, const char *method, bool seconds) -> qint64 {
        QDBusMessage msg = QDBusMessage::createMethodCall(QString::fromLatin1(service), QString::fromLatin1(path),
                                                          QString::fromLatin1(iface), QString::fromLatin1(method));
        const QDBusMessage reply = bus.call(msg, QDBus::Block, 300);
        if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty())
            return -1;
        bool ok = false;
        const qint64 v = reply.arguments().first().toLongLong(&ok);
        if (!ok || v < 0)
            return -1;
        return seconds ? v * 1000 : v;
    };

    // GNOME / Mutter (milliseconds)
    if (m_backend == 0 || m_backend == 1) {
        const qint64 v = call("org.gnome.Mutter.IdleMonitor", "/org/gnome/Mutter/IdleMonitor/Core",
                              "org.gnome.Mutter.IdleMonitor", "GetIdletime", false);
        if (v >= 0) {
            m_backend = 1;
            return v;
        }
    }
    // KDE and other freedesktop screensaver implementations (seconds)
    if (m_backend == 0 || m_backend == 2) {
        const qint64 v = call("org.freedesktop.ScreenSaver", "/ScreenSaver", "org.freedesktop.ScreenSaver",
                              "GetSessionIdleTime", true);
        if (v >= 0) {
            m_backend = 2;
            return v;
        }
    }
    m_backend = 0;
#endif
    return -1;
}

void IdleMonitor::poll()
{
    const qint64 ms = queryIdleMs();
    m_idleMs = ms;
    const bool supported = ms >= 0;
    if (!m_probed || supported != m_supported) {
        m_probed = true;
        m_supported = supported;
        emit supportedChanged(supported);
    }
    const bool idle = supported && ms >= m_thresholdMs;
    if (idle != m_idle) {
        m_idle = idle;
        emit idleChanged(idle);
    }
    // Poll faster while idle so the lights come back quickly on activity.
    m_timer.setInterval(idle ? 1000 : 5000);
}
