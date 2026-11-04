#include "devices/Rival3WirelessGen2.h"

#include "devices/Protocol.h"

using namespace Protocol::Rival3WirelessGen2;

namespace {
constexpr int kBatteryPollConnectedMs = 30000;
constexpr int kBatteryPollAsleepMs = 8000;
} // namespace

Rival3WirelessGen2::Rival3WirelessGen2(const HidDeviceInfo &info, HidTransport *transport, QObject *parent)
    : Device(info, transport, parent)
{
    m_commandDelayMs = CommandDelayMs;

    m_batteryPoll.setInterval(kBatteryPollConnectedMs);
    connect(&m_batteryPoll, &QTimer::timeout, this, &Rival3WirelessGen2::requestBattery);

    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(700);
    connect(&m_saveTimer, &QTimer::timeout, this, &Rival3WirelessGen2::saveNow);
}

QJsonObject Rival3WirelessGen2::defaultConfig() const
{
    return MouseConfig::defaults().toJson();
}

void Rival3WirelessGen2::applyConfig(const QJsonObject &data)
{
    m_config = data;
    MouseConfig next = MouseConfig::fromJson(data);
    next.normalize();
    const MouseConfig previous = m_mouseConfig;
    m_mouseConfig = next;
    if (m_opened)
        sendDiff(m_hasSent ? m_lastSent : previous, next, !m_hasSent);
    emit configApplied();
}

void Rival3WirelessGen2::sendDiff(const MouseConfig &previous, const MouseConfig &next, bool force)
{
    bool changed = false;

    const bool sensitivityChanged = force || previous.presets != next.presets || previous.activePreset != next.activePreset
        || previous.separateXY != next.separateXY;
    if (sensitivityChanged) {
        int active = 0;
        const auto presets = next.enabledPresets(&active);
        send(setSensitivity(presets, active));
        changed = true;
    }
    if (force || previous.pollingRate != next.pollingRate) {
        send(setPollingRate(next.pollingRate));
        changed = true;
    }
    if (force || previous.buttons != next.buttons) {
        send(setButtons(next.buttons));
        changed = true;
    }
    m_lastSent = next;
    m_hasSent = true;
    if (changed)
        scheduleSave();
}

void Rival3WirelessGen2::scheduleSave()
{
    m_saveTimer.start();
}

void Rival3WirelessGen2::saveNow()
{
    if (!m_opened)
        return;
    send(save());
    emit saved();
}

void Rival3WirelessGen2::resendAll()
{
    if (!m_opened)
        return;
    sendDiff(m_lastSent, m_mouseConfig, true);
}

void Rival3WirelessGen2::requestBattery()
{
    if (m_opened)
        query(queryBattery(), TagBattery, 64, 250);
}

void Rival3WirelessGen2::onOpened()
{
    m_ioErrors = 0;
    m_failedQueries = 0;
    m_hasSent = false;
    sendDiff(m_mouseConfig, m_mouseConfig, true);
    requestBattery();
    m_batteryPoll.start();
}

void Rival3WirelessGen2::onResponse(int tag, const QByteArray &data)
{
    if (tag != TagBattery)
        return;
    const BatteryStatus b = parseBattery(data);
    if (b.valid) {
        const bool wasAsleep = status() == Status::Asleep;
        m_failedQueries = 0;
        setBattery(b.level, b.charging);
        setStatus(Status::Connected);
        m_batteryPoll.setInterval(kBatteryPollConnectedMs);
        if (wasAsleep) {
            // The mouse was off while we talked to the receiver: make sure it
            // really has the current settings.
            resendAll();
        }
    } else {
        ++m_failedQueries;
        if (m_failedQueries == 1) {
            // Retry quickly once before declaring the mouse asleep.
            QTimer::singleShot(1500, this, &Rival3WirelessGen2::requestBattery);
        } else if (status() == Status::Connected) {
            setBattery(-1, false);
            setStatus(Status::Asleep);
            m_batteryPoll.setInterval(kBatteryPollAsleepMs);
        }
    }
}

void Rival3WirelessGen2::onIoError(const QString &error)
{
    if (++m_ioErrors >= 5) {
        m_batteryPoll.stop();
        setStatus(Status::Error, error);
    }
}
