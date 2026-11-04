#include "devices/Rival3WirelessGen2.h"

#include "core/IdleMonitor.h"
#include "devices/Protocol.h"

using namespace Protocol::Rival3WirelessGen2;

namespace {
// Every query is a vendor command the mouse firmware has to service, so keep
// the traffic low: nothing we send should make the cursor hitch.
constexpr int kBatteryPollConnectedMs = 60000;
constexpr int kBatteryPollAsleepMs = 20000;
// While the user is away we stay completely silent so the mouse can sleep and
// is not kept awake by our queries.
constexpr qint64 kUserIdleThresholdMs = 2 * 60 * 1000;
} // namespace

Rival3WirelessGen2::Rival3WirelessGen2(const HidDeviceInfo &info, HidTransport *transport, QObject *parent)
    : Device(info, transport, parent)
    , m_idle(new IdleMonitor(this))
{
    m_commandDelayMs = CommandDelayMs;

    m_batteryPoll.setInterval(kBatteryPollConnectedMs);
    connect(&m_batteryPoll, &QTimer::timeout, this, &Rival3WirelessGen2::requestBattery);

    m_idle->setThresholdMs(kUserIdleThresholdMs);
    connect(m_idle, &IdleMonitor::idleChanged, this, [this](bool idle) {
        if (!idle && m_opened)
            QTimer::singleShot(2000, this, &Rival3WirelessGen2::requestBattery);
    });

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
    if (changed) {
        // If the mouse is asleep the receiver may not deliver this; repeat it
        // once the mouse answers again.
        if (status() == Status::Asleep)
            m_resendOnWake = true;
        scheduleSave();
    }
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
    if (!m_opened)
        return;
    // Nobody is using the computer: let the mouse sleep undisturbed.
    if (m_idle->isSupported() && m_idle->isIdle())
        return;
    query(queryBattery(), TagBattery, 64, 250);
}

void Rival3WirelessGen2::onOpened()
{
    m_ioErrors = 0;
    m_failedQueries = 0;
    m_hasSent = false;
    sendDiff(m_mouseConfig, m_mouseConfig, true);
    m_idle->setEnabled(true);
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
        if (wasAsleep && m_resendOnWake) {
            // Settings were changed while the mouse was off: deliver them now.
            // (Re-sending unconditionally on every wake-up would write the
            // on-board memory each time and make the cursor stutter briefly.)
            m_resendOnWake = false;
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
