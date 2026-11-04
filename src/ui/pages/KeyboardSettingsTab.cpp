#include "ui/pages/KeyboardSettingsTab.h"

#include "core/IdleMonitor.h"
#include "core/Theme.h"
#include "devices/Apex3Tkl.h"
#include "ui/pages/PageHelpers.h"
#include "ui/widgets/Card.h"
#include "ui/widgets/ToggleSwitch.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

KeyboardSettingsTab::KeyboardSettingsTab(Apex3Tkl *device, QWidget *parent)
    : ConfigTab(parent)
    , m_device(device)
{
    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(16);

    // Brightness keys
    auto *brightness = new Card(tr("Brightness keys"), QString(), this);
    m_followBrightness = new ToggleSwitch(brightness);
    brightness->addRow(tr("Follow the keyboard's brightness keys"), m_followBrightness,
                       tr("When you change the brightness with the keys on the keyboard, the slider and the saved config follow."));
    v->addWidget(brightness);

    // Power / idle
    auto *power = new Card(tr("Power"), QString(), this);
    m_restore = new ToggleSwitch(power);
    power->addRow(tr("Restore illumination on reconnect"), m_restore,
                  tr("The Apex 3 TKL forgets custom lighting when it loses power. Re-apply the active config whenever it reconnects."));
    power->addWidget(PageHelpers::divider(power));

    auto *idleControls = new QWidget(power);
    auto *ih = new QHBoxLayout(idleControls);
    ih->setContentsMargins(0, 0, 0, 0);
    ih->setSpacing(10);
    m_idleMinutes = new QSpinBox(idleControls);
    m_idleMinutes->setRange(1, 120);
    m_idleMinutes->setSuffix(tr(" min"));
    m_idleMinutes->setFixedWidth(84);
    m_idleMinutes->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ih->addWidget(m_idleMinutes);
    m_idleEnabled = new ToggleSwitch(idleControls);
    ih->addWidget(m_idleEnabled);
    power->addRow(tr("Turn illumination off when idle"), idleControls,
                  tr("Lights go off after the chosen period without keyboard or mouse activity and come back instantly."));
    m_idleSupport = PageHelpers::secondary(QString(), power);
    m_idleSupport->setFont(Theme::font(12));
    m_idleSupport->setVisible(false);
    power->addWidget(m_idleSupport);
    v->addWidget(power);

    // Device info
    auto *info = new Card(tr("Device information"), QString(), this);
    auto addInfo = [info](const QString &k, const QString &val) {
        auto *l = new QLabel(val, info);
        l->setProperty("role", QStringLiteral("secondary"));
        l->setTextInteractionFlags(Qt::TextSelectableByMouse);
        info->addRow(k, l);
    };
    addInfo(tr("Model"), device->modelName());
    addInfo(tr("USB ID"), QStringLiteral("%1:%2").arg(device->info().vendorId, 4, 16, QLatin1Char('0')).arg(device->info().productId, 4, 16, QLatin1Char('0')));
    addInfo(tr("HID interface"), QString::number(device->info().interfaceNumber));
    addInfo(tr("Device path"), device->path());
    addInfo(tr("Illumination zones"), QStringLiteral("8"));
    addInfo(tr("On-board memory"), tr("No (lighting is kept alive by this app)"));
    v->addWidget(info);
    v->addStretch(1);

    connect(m_followBrightness, &QAbstractButton::toggled, this, [this](bool on) {
        if (m_loading)
            return;
        m_cfg.followHardwareBrightness = on;
        emitChange();
    });
    connect(m_restore, &QAbstractButton::toggled, this, [this](bool on) {
        if (m_loading)
            return;
        m_cfg.restoreOnConnect = on;
        emitChange();
    });
    connect(m_idleEnabled, &QAbstractButton::toggled, this, [this](bool on) {
        if (m_loading)
            return;
        m_cfg.idleDimEnabled = on;
        m_idleMinutes->setEnabled(on);
        emitChange();
    });
    connect(m_idleMinutes, &QSpinBox::editingFinished, this, [this]() {
        if (m_loading || m_idleMinutes->value() == m_cfg.idleMinutes)
            return;
        m_cfg.idleMinutes = m_idleMinutes->value();
        emitChange();
    });
    connect(device->idleMonitor(), &IdleMonitor::supportedChanged, this, [this](bool supported) {
        m_idleSupport->setVisible(!supported && m_cfg.idleDimEnabled);
        m_idleSupport->setText(tr("Idle detection is not available in this desktop session (needs GNOME or a freedesktop.org ScreenSaver D-Bus service)."));
    });

    loadConfig(device->currentConfig().isEmpty() ? device->defaultConfig() : device->currentConfig());
}

void KeyboardSettingsTab::loadConfig(const QJsonObject &data)
{
    m_loading = true;
    m_cfg = KeyboardConfig::fromJson(data);
    m_followBrightness->setChecked(m_cfg.followHardwareBrightness);
    m_restore->setChecked(m_cfg.restoreOnConnect);
    m_idleEnabled->setChecked(m_cfg.idleDimEnabled);
    m_idleMinutes->setValue(m_cfg.idleMinutes);
    m_idleMinutes->setEnabled(m_cfg.idleDimEnabled);
    const IdleMonitor *mon = m_device->idleMonitor();
    m_idleSupport->setText(tr("Idle detection is not available in this desktop session (needs GNOME or a freedesktop.org ScreenSaver D-Bus service)."));
    m_idleSupport->setVisible(m_cfg.idleDimEnabled && mon->hasProbed() && !mon->isSupported());
    m_loading = false;
}

void KeyboardSettingsTab::emitChange()
{
    if (m_loading)
        return;
    emit configEdited(m_cfg.toJson());
}
