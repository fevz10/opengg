#pragma once

#include <QObject>
#include <QTimer>

/**
 * Reports how long the user has been idle (no keyboard/mouse input) using the
 * desktop session's D-Bus interfaces. Used to dim/turn off the keyboard
 * illumination after a period of inactivity, something the Apex 3 TKL cannot
 * do on its own.
 *
 * Supported backends (tried in order):
 *   - org.gnome.Mutter.IdleMonitor (GNOME, Wayland + X11)
 *   - org.freedesktop.ScreenSaver.GetSessionIdleTime (KDE Plasma and others)
 */
class IdleMonitor : public QObject
{
    Q_OBJECT
public:
    explicit IdleMonitor(QObject *parent = nullptr);

    /// Whether any backend answered at least once.
    bool isSupported() const { return m_supported; }
    /// Whether a backend probe has run yet.
    bool hasProbed() const { return m_probed; }
    /// Current idle time in milliseconds (-1 when unknown).
    qint64 idleMs() const { return m_idleMs; }

    void setEnabled(bool enabled);
    void setThresholdMs(qint64 thresholdMs) { m_thresholdMs = thresholdMs; }
    bool isIdle() const { return m_idle; }

signals:
    void idleChanged(bool idle);
    void supportedChanged(bool supported);

private slots:
    void poll();

private:
    qint64 queryIdleMs();

    QTimer m_timer;
    qint64 m_idleMs = -1;
    qint64 m_thresholdMs = 5 * 60 * 1000;
    bool m_idle = false;
    bool m_supported = false;
    bool m_probed = false;
};
