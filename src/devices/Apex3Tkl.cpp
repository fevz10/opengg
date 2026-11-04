#include "devices/Apex3Tkl.h"

#include "core/IdleMonitor.h"
#include "devices/Protocol.h"

using namespace Protocol::Apex3Tkl;

Apex3Tkl::Apex3Tkl(const HidDeviceInfo &info, HidTransport *transport, QObject *parent)
    : Device(info, transport, parent)
    , m_engine(new LightingEngine(this))
    , m_idle(new IdleMonitor(this))
{
    m_commandDelayMs = CommandDelayMs;
    m_engine->setActive(false);

    connect(m_engine, &LightingEngine::deviceFrame, this, [this](const ZoneColors &colors) {
        if (m_opened)
            send(setColors(colors));
    });
    connect(m_engine, &LightingEngine::hardwareRainbowRequested, this, [this]() {
        if (m_opened)
            send(setRainbowWave());
    });
    connect(m_engine, &LightingEngine::brightnessRequested, this, [this](int level) {
        m_expectedLevel = level;
        if (m_opened)
            send(setBrightness(level));
    });
    connect(m_idle, &IdleMonitor::idleChanged, this, [this](bool idle) { m_engine->setSuspended(idle); });

    m_brightnessPoll.setInterval(3000);
    connect(&m_brightnessPoll, &QTimer::timeout, this, &Apex3Tkl::requestBrightness);
}

QJsonObject Apex3Tkl::defaultConfig() const
{
    return KeyboardConfig::defaults().toJson();
}

void Apex3Tkl::applyConfig(const QJsonObject &data)
{
    m_config = data;
    m_keyboardConfig = KeyboardConfig::fromJson(data);
    m_engine->setConfig(m_keyboardConfig);
    updateMonitors();
    emit configApplied();
}

void Apex3Tkl::updateMonitors()
{
    if (m_keyboardConfig.idleDimEnabled && m_opened) {
        m_idle->setThresholdMs(qint64(m_keyboardConfig.idleMinutes) * 60 * 1000);
        m_idle->setEnabled(true);
    } else {
        m_idle->setEnabled(false);
    }
    if (m_keyboardConfig.followHardwareBrightness && m_opened) {
        if (!m_brightnessPoll.isActive())
            m_brightnessPoll.start();
    } else {
        m_brightnessPoll.stop();
    }
}

void Apex3Tkl::requestBrightness()
{
    if (m_opened)
        query(queryBrightness(), TagBrightness, 64, 150);
}

void Apex3Tkl::onOpened()
{
    m_ioErrors = 0;
    m_engine->setActive(true); // re-sends brightness + current frame
    updateMonitors();
}

void Apex3Tkl::onResponse(int tag, const QByteArray &data)
{
    if (tag != TagBrightness)
        return;
    const int level = parseBrightness(data);
    if (level < 0)
        return;
    if (m_expectedLevel >= 0 && level != m_expectedLevel) {
        // Changed with Fn+F11/F12 on the keyboard: adopt it.
        m_expectedLevel = level;
        emit hardwareBrightnessChanged(levelToPercent(level));
    } else if (m_expectedLevel < 0) {
        m_expectedLevel = level;
    }
}

void Apex3Tkl::onIoError(const QString &error)
{
    if (++m_ioErrors >= 5) {
        m_engine->setActive(false);
        m_brightnessPoll.stop();
        setStatus(Status::Error, error);
    }
}
