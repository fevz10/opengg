#include "core/IdleMonitor.h"

#if defined(SSGG_HAVE_DBUS)
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
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

    // GNOME / Mutter
    {
        QDBusInterface mutter(QStringLiteral("org.gnome.Mutter.IdleMonitor"),
                              QStringLiteral("/org/gnome/Mutter/IdleMonitor/Core"),
                              QStringLiteral("org.gnome.Mutter.IdleMonitor"), bus);
        if (mutter.isValid()) {
            QDBusReply<qulonglong> reply = mutter.call(QStringLiteral("GetIdletime"));
            if (reply.isValid())
                return qint64(reply.value());
        }
    }
    // KDE and other freedesktop screensaver implementations (seconds)
    {
        QDBusInterface ss(QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("/ScreenSaver"),
                          QStringLiteral("org.freedesktop.ScreenSaver"), bus);
        if (ss.isValid()) {
            QDBusReply<uint> reply = ss.call(QStringLiteral("GetSessionIdleTime"));
            if (reply.isValid())
                return qint64(reply.value()) * 1000;
        }
    }
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
