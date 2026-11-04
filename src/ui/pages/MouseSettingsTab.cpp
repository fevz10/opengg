#include "ui/pages/MouseSettingsTab.h"

#include "core/ConfigStore.h"
#include "core/Icons.h"
#include "core/Theme.h"
#include "devices/Rival3WirelessGen2.h"
#include "ui/pages/PageHelpers.h"
#include "ui/widgets/Card.h"
#include "ui/widgets/SegmentedControl.h"
#include "ui/widgets/Toast.h"
#include "ui/widgets/ToggleSwitch.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
const int kPollingRates[] = {125, 250, 500, 1000};
}

MouseSettingsTab::MouseSettingsTab(Rival3WirelessGen2 *device, const AppContext &ctx, QWidget *parent)
    : ConfigTab(parent)
    , m_device(device)
    , m_ctx(ctx)
{
    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(16);

    // Polling rate
    auto *polling = new Card(tr("Polling rate"), tr("How often the mouse reports to the computer. 1000 Hz is the most responsive; lower rates extend battery life."), this);
    m_polling = new SegmentedControl(polling);
    m_polling->setItems({QStringLiteral("125 Hz"), QStringLiteral("250 Hz"), QStringLiteral("500 Hz"), QStringLiteral("1000 Hz")});
    auto *ph = new QHBoxLayout;
    ph->addWidget(m_polling, 0, Qt::AlignLeft);
    ph->addStretch(1);
    polling->addLayout(ph);
    v->addWidget(polling);

    // Battery
    auto *battery = new Card(tr("Battery"), QString(), this);
    auto *refresh = new QPushButton(tr("Refresh"), battery);
    refresh->setProperty("variant", QStringLiteral("ghost"));
    refresh->setIcon(Icons::icon(QStringLiteral("refresh"), Theme::TextSecondary, 16));
    refresh->setCursor(Qt::PointingHandCursor);
    connect(refresh, &QPushButton::clicked, device, &Rival3WirelessGen2::requestBattery);
    battery->setHeaderWidget(refresh);
    auto *bRow = new QHBoxLayout;
    bRow->setSpacing(18);
    m_batteryValue = new QLabel(battery);
    m_batteryValue->setFont(Theme::titleFont(34));
    bRow->addWidget(m_batteryValue);
    auto *bCol = new QVBoxLayout;
    bCol->setSpacing(8);
    m_batteryState = PageHelpers::secondary(QString(), battery);
    bCol->addWidget(m_batteryState);
    m_batteryBar = new QProgressBar(battery);
    m_batteryBar->setRange(0, 100);
    m_batteryBar->setTextVisible(false);
    m_batteryBar->setFixedHeight(6);
    bCol->addWidget(m_batteryBar);
    bRow->addLayout(bCol, 1);
    battery->addLayout(bRow);
    battery->addWidget(PageHelpers::divider(battery));
    m_lowBattery = new ToggleSwitch(battery);
    battery->addRow(tr("Low battery notification"), m_lowBattery, tr("Show a notification when the level drops to 15%."));
    v->addWidget(battery);

    // On-board memory
    auto *memory = new Card(tr("On-board memory"), tr("CPI presets, polling rate and button mapping are stored on the mouse and apply on any computer, even without this app."), this);
    auto *mRow = new QHBoxLayout;
    mRow->setSpacing(10);
    auto *saveNow = new QPushButton(tr("Save to mouse now"), memory);
    saveNow->setCursor(Qt::PointingHandCursor);
    connect(saveNow, &QPushButton::clicked, this, [this]() {
        m_device->resendAll();
        if (m_ctx.toasts)
            m_ctx.toasts->show(tr("Settings written to %1").arg(m_device->displayName()), Toast::Success);
    });
    mRow->addWidget(saveNow);
    auto *reset = new QPushButton(tr("Restore factory defaults"), memory);
    reset->setProperty("variant", QStringLiteral("danger"));
    reset->setCursor(Qt::PointingHandCursor);
    connect(reset, &QPushButton::clicked, this, [this]() {
        const auto answer = QMessageBox::question(this, tr("Restore defaults"),
                                                  tr("Reset the active config of %1 to the factory defaults?").arg(m_device->displayName()),
                                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
        m_cfg = MouseConfig::defaults();
        emitChange();
    });
    mRow->addWidget(reset);
    mRow->addStretch(1);
    memory->addLayout(mRow);
    v->addWidget(memory);

    // Device info
    auto *info = new Card(tr("Device information"), QString(), this);
    auto addInfo = [info](const QString &k, const QString &val) {
        auto *l = new QLabel(val, info);
        l->setProperty("role", QStringLiteral("secondary"));
        l->setTextInteractionFlags(Qt::TextSelectableByMouse);
        info->addRow(k, l);
    };
    addInfo(tr("Model"), device->modelName());
    addInfo(tr("Connection"), tr("2.4 GHz wireless receiver"));
    addInfo(tr("USB ID"), QStringLiteral("%1:%2").arg(device->info().vendorId, 4, 16, QLatin1Char('0')).arg(device->info().productId, 4, 16, QLatin1Char('0')));
    addInfo(tr("HID interface"), QString::number(device->info().interfaceNumber));
    addInfo(tr("Device path"), device->path());
    addInfo(tr("Sensor"), tr("TrueMove Air, 100 – 18 000 CPI"));
    v->addWidget(info);
    v->addStretch(1);

    connect(m_polling, &SegmentedControl::currentIndexChanged, this, [this](int idx) {
        if (m_loading)
            return;
        m_cfg.pollingRate = kPollingRates[qBound(0, idx, 3)];
        emitChange();
    });
    connect(m_lowBattery, &QAbstractButton::toggled, this, [this](bool on) {
        if (m_loading)
            return;
        m_cfg.lowBatteryWarning = on;
        emitChange();
    });
    connect(device, &Device::batteryChanged, this, &MouseSettingsTab::refreshBattery);
    connect(device, &Device::statusChanged, this, &MouseSettingsTab::refreshBattery);

    loadConfig(device->currentConfig().isEmpty() ? device->defaultConfig() : device->currentConfig());
    refreshBattery();
}

void MouseSettingsTab::refreshBattery()
{
    const int lvl = m_device->batteryLevel();
    if (lvl < 0) {
        m_batteryValue->setText(QStringLiteral("—"));
        m_batteryBar->setValue(0);
        m_batteryState->setText(m_device->status() == Device::Status::Asleep
                                    ? tr("The mouse is asleep or switched off. Move it to wake it up.")
                                    : tr("Battery level not reported yet."));
        return;
    }
    m_batteryValue->setText(QStringLiteral("%1%").arg(lvl));
    m_batteryBar->setValue(lvl);
    const QColor c = lvl <= 15 ? Theme::Danger : (lvl <= 30 ? Theme::Warning : Theme::Success);
    m_batteryBar->setStyleSheet(QStringLiteral("QProgressBar::chunk{background:%1;border-radius:3px;}").arg(Theme::hex(c)));
    m_batteryState->setText(m_device->isCharging() ? tr("Charging") : tr("Running on battery"));
}

void MouseSettingsTab::loadConfig(const QJsonObject &data)
{
    m_loading = true;
    m_cfg = MouseConfig::fromJson(data);
    int idx = 3;
    for (int i = 0; i < 4; ++i)
        if (kPollingRates[i] == m_cfg.pollingRate)
            idx = i;
    m_polling->setCurrentIndex(idx);
    m_lowBattery->setChecked(m_cfg.lowBatteryWarning);
    m_loading = false;
}

void MouseSettingsTab::emitChange()
{
    if (m_loading)
        return;
    emit configEdited(m_cfg.toJson());
}
