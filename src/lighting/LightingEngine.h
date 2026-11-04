#pragma once

#include "devices/Protocol.h"
#include "models/KeyboardConfig.h"

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

/**
 * Turns a KeyboardConfig into frames of zone colors.
 *
 * The Apex 3 TKL only knows two hardware modes (direct colors and a rainbow
 * wave), so "Breathe" and "Color Shift" are rendered here in software, the
 * same way SteelSeries GG does it on Windows.
 */
class LightingEngine : public QObject
{
    Q_OBJECT
public:
    using ZoneColors = Protocol::Apex3Tkl::ZoneColors;

    explicit LightingEngine(QObject *parent = nullptr);

    void setConfig(const KeyboardConfig &config);
    const KeyboardConfig &config() const { return m_config; }

    /// Suspends output (lights off) e.g. while the user is idle.
    void setSuspended(bool suspended);
    bool isSuspended() const { return m_suspended; }

    /// Stops/starts emitting frames at all (device gone / back).
    void setActive(bool active);

    ZoneColors lastPreviewFrame() const { return m_lastPreview; }
    int hardwareBrightnessLevel() const;

    /// Re-sends the current state (used after a reconnect).
    void resend();

signals:
    /// Colors that must be written to the keyboard (direct mode).
    void deviceFrame(const Protocol::Apex3Tkl::ZoneColors &colors);
    /// Colors for the on screen preview (also emitted for hardware effects).
    void previewFrame(const Protocol::Apex3Tkl::ZoneColors &colors);
    /// The hardware rainbow wave must be enabled.
    void hardwareRainbowRequested();
    /// Brightness level (0..16) to send to the keyboard.
    void brightnessRequested(int level);

private slots:
    void tick();

private:
    void restart();
    ZoneColors renderSoftware(double seconds) const;
    ZoneColors renderRainbowPreview(double seconds) const;
    static QColor gradientAt(const QList<QColor> &stops, double t);

    KeyboardConfig m_config;
    QTimer m_timer;
    QElapsedTimer m_clock;
    ZoneColors m_lastPreview{};
    bool m_suspended = false;
    bool m_active = true;
    int m_lastBrightnessLevel = -1;
};
