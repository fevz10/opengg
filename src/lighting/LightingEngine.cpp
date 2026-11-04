#include "lighting/LightingEngine.h"

#include <QtMath>

#include <algorithm>

namespace {
constexpr int kFrameIntervalMs = 50; // 20 fps
constexpr QRgb kBlack = 0xff000000;

QRgb scaled(const QColor &c, double factor)
{
    factor = std::clamp(factor, 0.0, 1.0);
    return qRgb(int(c.red() * factor + 0.5), int(c.green() * factor + 0.5), int(c.blue() * factor + 0.5));
}
} // namespace

LightingEngine::LightingEngine(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(kFrameIntervalMs);
    connect(&m_timer, &QTimer::timeout, this, &LightingEngine::tick);
    m_clock.start();
    m_lastPreview.fill(kBlack);
}

int LightingEngine::hardwareBrightnessLevel() const
{
    return Protocol::Apex3Tkl::percentToLevel(m_config.brightness);
}

void LightingEngine::setConfig(const KeyboardConfig &config)
{
    const bool effectChanged = config.effect != m_config.effect;
    m_config = config;
    const int level = hardwareBrightnessLevel();
    if (level != m_lastBrightnessLevel) {
        m_lastBrightnessLevel = level;
        if (m_active && !m_suspended)
            emit brightnessRequested(level);
    }
    if (effectChanged)
        m_clock.restart();
    restart();
}

void LightingEngine::setSuspended(bool suspended)
{
    if (m_suspended == suspended)
        return;
    m_suspended = suspended;
    if (suspended) {
        m_timer.stop();
        ZoneColors off{};
        off.fill(kBlack);
        if (m_active)
            emit deviceFrame(off);
    } else {
        resend();
    }
}

void LightingEngine::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    if (!active)
        m_timer.stop();
    else
        resend();
}

void LightingEngine::resend()
{
    m_lastBrightnessLevel = hardwareBrightnessLevel();
    if (m_active && !m_suspended)
        emit brightnessRequested(m_lastBrightnessLevel);
    restart();
}

void LightingEngine::restart()
{
    m_timer.stop();
    if (!m_active)
        return;

    switch (m_config.effect) {
    case KeyboardConfig::Steady: {
        ZoneColors f{};
        for (int i = 0; i < KeyboardConfig::ZoneCount; ++i)
            f[size_t(i)] = m_config.zoneColors[size_t(i)].rgb();
        m_lastPreview = f;
        emit previewFrame(f);
        if (!m_suspended)
            emit deviceFrame(f);
        break;
    }
    case KeyboardConfig::Off: {
        ZoneColors f{};
        f.fill(kBlack);
        m_lastPreview = f;
        emit previewFrame(f);
        if (!m_suspended)
            emit deviceFrame(f);
        break;
    }
    case KeyboardConfig::RainbowWave:
        if (!m_suspended)
            emit hardwareRainbowRequested();
        m_timer.start();
        tick();
        break;
    case KeyboardConfig::Breathe:
    case KeyboardConfig::ColorShift:
        if (!m_suspended)
            m_timer.start();
        tick();
        break;
    }
}

QColor LightingEngine::gradientAt(const QList<QColor> &stops, double t)
{
    if (stops.isEmpty())
        return QColor(Qt::black);
    if (stops.size() == 1)
        return stops.first();
    t -= std::floor(t);
    const double scaled = t * stops.size();
    const int i = int(scaled) % stops.size();
    const int j = (i + 1) % stops.size();
    const double f = scaled - std::floor(scaled);
    const QColor &a = stops.at(i);
    const QColor &b = stops.at(j);
    return QColor(int(a.red() + (b.red() - a.red()) * f + 0.5), int(a.green() + (b.green() - a.green()) * f + 0.5),
                  int(a.blue() + (b.blue() - a.blue()) * f + 0.5));
}

LightingEngine::ZoneColors LightingEngine::renderSoftware(double seconds) const
{
    ZoneColors f{};
    const double speed = m_config.speed / 100.0; // 0..1
    if (m_config.effect == KeyboardConfig::Breathe) {
        const double period = 6.0 - 5.0 * speed; // 6s .. 1s
        const double phase = std::fmod(seconds, period) / period;
        const double factor = 0.5 * (1.0 - std::cos(phase * 2.0 * M_PI));
        const double level = 0.04 + 0.96 * factor;
        for (int i = 0; i < KeyboardConfig::ZoneCount; ++i)
            f[size_t(i)] = scaled(m_config.zoneColors[size_t(i)], level);
    } else { // ColorShift
        const double period = 20.0 - 17.0 * speed; // 20s .. 3s
        const double base = seconds / period;
        for (int i = 0; i < KeyboardConfig::ZoneCount; ++i) {
            const double t = base - double(i) / (KeyboardConfig::ZoneCount * 1.5);
            f[size_t(i)] = gradientAt(m_config.shiftColors, t).rgb();
        }
    }
    return f;
}

LightingEngine::ZoneColors LightingEngine::renderRainbowPreview(double seconds) const
{
    ZoneColors f{};
    const double base = std::fmod(seconds / 4.0, 1.0);
    for (int i = 0; i < KeyboardConfig::ZoneCount; ++i) {
        double hue = base - double(i) / KeyboardConfig::ZoneCount;
        hue -= std::floor(hue);
        f[size_t(i)] = QColor::fromHsvF(hue, 1.0, 1.0).rgb();
    }
    return f;
}

void LightingEngine::tick()
{
    const double seconds = m_clock.elapsed() / 1000.0;
    if (m_config.effect == KeyboardConfig::RainbowWave) {
        m_lastPreview = renderRainbowPreview(seconds);
        emit previewFrame(m_lastPreview);
        return;
    }
    m_lastPreview = renderSoftware(seconds);
    emit previewFrame(m_lastPreview);
    if (!m_suspended && m_active)
        emit deviceFrame(m_lastPreview);
}
