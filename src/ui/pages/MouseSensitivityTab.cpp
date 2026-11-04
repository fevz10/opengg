#include "ui/pages/MouseSensitivityTab.h"

#include "core/Theme.h"
#include "devices/Protocol.h"
#include "devices/Rival3WirelessGen2.h"
#include "ui/pages/PageHelpers.h"
#include "ui/widgets/Card.h"
#include "ui/widgets/ToggleSwitch.h"
#include "ui/widgets/ValueSlider.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

using namespace Protocol::Rival3WirelessGen2;

MouseSensitivityTab::MouseSensitivityTab(Rival3WirelessGen2 *device, QWidget *parent)
    : ConfigTab(parent)
    , m_device(device)
    , m_activeGroup(new QButtonGroup(this))
{
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(160);
    connect(&m_debounce, &QTimer::timeout, this, &MouseSensitivityTab::emitChange);

    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(16);

    auto *card = new Card(tr("CPI presets"), tr("Up to five presets, %1 – %2 CPI in steps of %3. The CPI button cycles through the enabled presets.")
                                                  .arg(DpiMin).arg(DpiMax).arg(DpiStep),
                          this);
    auto *xyBox = new QWidget(card);
    auto *xyh = new QHBoxLayout(xyBox);
    xyh->setContentsMargins(0, 0, 0, 0);
    xyh->setSpacing(8);
    auto *xyLabel = new QLabel(tr("Separate X / Y"), xyBox);
    xyLabel->setProperty("role", QStringLiteral("secondary"));
    xyh->addWidget(xyLabel);
    m_separateXY = new ToggleSwitch(xyBox);
    xyh->addWidget(m_separateXY);
    card->setHeaderWidget(xyBox);

    m_activeGroup->setExclusive(true);
    for (int i = 0; i < MouseConfig::PresetCount; ++i) {
        Row &r = m_rows[size_t(i)];
        r.widget = new QWidget(card);
        auto *rh = new QHBoxLayout(r.widget);
        rh->setContentsMargins(0, 8, 0, 8);
        rh->setSpacing(14);

        r.enabled = new ToggleSwitch(r.widget);
        r.enabled->setToolTip(tr("Enable preset"));
        rh->addWidget(r.enabled, 0, Qt::AlignTop);

        auto *badge = new QLabel(QString::number(i + 1), r.widget);
        badge->setFixedSize(26, 26);
        badge->setAlignment(Qt::AlignCenter);
        badge->setFont(Theme::font(12, QFont::Bold));
        badge->setStyleSheet(QStringLiteral("background:%1; border-radius:13px;").arg(Theme::hex(Theme::Bg4)));
        rh->addWidget(badge, 0, Qt::AlignTop);

        r.active = new QRadioButton(tr("Active"), r.widget);
        r.active->setCursor(Qt::PointingHandCursor);
        r.active->setFixedWidth(78);
        m_activeGroup->addButton(r.active, i);
        rh->addWidget(r.active, 0, Qt::AlignTop);

        auto *sliders = new QVBoxLayout;
        sliders->setSpacing(6);
        auto *xRow = new QHBoxLayout;
        xRow->setSpacing(10);
        r.xLabel = new QLabel(QStringLiteral("CPI"), r.widget);
        r.xLabel->setProperty("role", QStringLiteral("muted"));
        r.xLabel->setFixedWidth(24);
        xRow->addWidget(r.xLabel);
        r.x = new ValueSlider(DpiMin, DpiMax, DpiStep, r.widget);
        r.x->setEditable(true);
        r.x->setLabelWidth(60);
        xRow->addWidget(r.x, 1);
        sliders->addLayout(xRow);

        r.yRow = new QWidget(r.widget);
        auto *yRow = new QHBoxLayout(r.yRow);
        yRow->setContentsMargins(0, 0, 0, 0);
        yRow->setSpacing(10);
        auto *yl = new QLabel(QStringLiteral("Y"), r.yRow);
        yl->setProperty("role", QStringLiteral("muted"));
        yl->setFixedWidth(24);
        yRow->addWidget(yl);
        r.y = new ValueSlider(DpiMin, DpiMax, DpiStep, r.yRow);
        r.y->setEditable(true);
        r.y->setLabelWidth(60);
        yRow->addWidget(r.y, 1);
        r.yRow->setVisible(false);
        sliders->addWidget(r.yRow);
        rh->addLayout(sliders, 1);

        card->addWidget(r.widget);
        if (i < MouseConfig::PresetCount - 1)
            card->addWidget(PageHelpers::divider(card));

        connect(r.enabled, &QAbstractButton::toggled, this, [this, i](bool on) {
            if (m_loading)
                return;
            MouseConfig::Preset &p = m_cfg.presets[size_t(i)];
            if (!on && m_cfg.enabledCount() == 1 && p.enabled) {
                // keep at least one preset
                m_loading = true;
                m_rows[size_t(i)].enabled->setChecked(true);
                m_loading = false;
                return;
            }
            p.enabled = on;
            m_cfg.normalize();
            refreshRows();
            emitChange();
        });
        connect(r.x, &ValueSlider::valueChanged, this, [this, i](int val) {
            if (m_loading)
                return;
            m_cfg.presets[size_t(i)].x = val;
            if (!m_cfg.separateXY)
                m_cfg.presets[size_t(i)].y = val;
            updateSummary();
            scheduleEmit();
        });
        connect(r.y, &ValueSlider::valueChanged, this, [this, i](int val) {
            if (m_loading)
                return;
            m_cfg.presets[size_t(i)].y = val;
            updateSummary();
            scheduleEmit();
        });
    }
    connect(m_activeGroup, &QButtonGroup::idClicked, this, [this](int id) {
        if (m_loading)
            return;
        m_cfg.activePreset = id;
        m_cfg.normalize();
        refreshRows();
        emitChange();
    });
    connect(m_separateXY, &QAbstractButton::toggled, this, [this](bool on) {
        if (m_loading)
            return;
        m_cfg.separateXY = on;
        if (!on)
            for (MouseConfig::Preset &p : m_cfg.presets)
                p.y = p.x;
        refreshRows();
        emitChange();
    });
    v->addWidget(card);

    auto *summaryCard = new Card(this);
    auto *sh = new QHBoxLayout;
    auto *sl = new QLabel(tr("Current sensitivity"), summaryCard);
    sl->setProperty("role", QStringLiteral("secondary"));
    sh->addWidget(sl, 1);
    m_summary = new QLabel(summaryCard);
    m_summary->setFont(Theme::titleFont(22));
    m_summary->setProperty("role", QStringLiteral("accent"));
    sh->addWidget(m_summary);
    summaryCard->addLayout(sh);
    v->addWidget(summaryCard);
    v->addStretch(1);

    loadConfig(device->currentConfig().isEmpty() ? device->defaultConfig() : device->currentConfig());
}

void MouseSensitivityTab::refreshRows()
{
    const bool was = m_loading;
    m_loading = true;
    m_separateXY->setChecked(m_cfg.separateXY);
    for (int i = 0; i < MouseConfig::PresetCount; ++i) {
        Row &r = m_rows[size_t(i)];
        const MouseConfig::Preset &p = m_cfg.presets[size_t(i)];
        r.enabled->setChecked(p.enabled);
        r.active->setEnabled(p.enabled);
        r.active->setChecked(i == m_cfg.activePreset);
        r.x->setValue(p.x);
        r.y->setValue(p.y);
        r.x->setEnabled(p.enabled);
        r.y->setEnabled(p.enabled);
        r.yRow->setVisible(m_cfg.separateXY);
        r.xLabel->setText(m_cfg.separateXY ? QStringLiteral("X") : QStringLiteral("CPI"));
    }
    updateSummary();
    m_loading = was;
}

void MouseSensitivityTab::updateSummary()
{
    const MouseConfig::Preset &p = m_cfg.presets[size_t(qBound(0, m_cfg.activePreset, MouseConfig::PresetCount - 1))];
    if (m_cfg.separateXY && p.x != p.y)
        m_summary->setText(tr("%1 × %2 CPI").arg(p.x).arg(p.y));
    else
        m_summary->setText(tr("%1 CPI").arg(p.x));
}

void MouseSensitivityTab::loadConfig(const QJsonObject &data)
{
    m_loading = true;
    m_cfg = MouseConfig::fromJson(data);
    refreshRows();
    m_loading = false;
}

void MouseSensitivityTab::scheduleEmit()
{
    m_debounce.start();
}

void MouseSensitivityTab::emitChange()
{
    if (m_loading)
        return;
    m_debounce.stop();
    m_cfg.normalize();
    emit configEdited(m_cfg.toJson());
}
