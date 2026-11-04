#include "ui/widgets/DeviceCard.h"

#include "core/Icons.h"
#include "core/Theme.h"
#include "devices/Apex3Tkl.h"
#include "devices/Device.h"
#include "lighting/LightingEngine.h"
#include "ui/widgets/KeyboardPreview.h"
#include "ui/widgets/MousePreview.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>

DeviceCard::DeviceCard(Device *device, QWidget *parent)
    : QAbstractButton(parent)
    , m_device(device)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::TabFocus);
    setFixedSize(sizeHint());

    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(18, 18, 18, 16);
    v->setSpacing(10);

    auto *previewBox = new QWidget(this);
    previewBox->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto *pv = new QVBoxLayout(previewBox);
    pv->setContentsMargins(0, 0, 0, 0);
    if (device->kind() == Device::Kind::Keyboard) {
        m_keyboard = new KeyboardPreview(previewBox);
        m_keyboard->setInteractive(false);
        m_keyboard->setAttribute(Qt::WA_TransparentForMouseEvents);
        pv->addStretch(1);
        pv->addWidget(m_keyboard);
        pv->addStretch(1);
        if (auto *kb = qobject_cast<Apex3Tkl *>(device)) {
            connect(kb->engine(), &LightingEngine::previewFrame, m_keyboard, &KeyboardPreview::setFrame);
            m_keyboard->setFrame(kb->engine()->lastPreviewFrame());
        }
    } else {
        m_mouse = new MousePreview(previewBox);
        m_mouse->setShowCallouts(false);
        m_mouse->setAttribute(Qt::WA_TransparentForMouseEvents);
        auto *h = new QHBoxLayout;
        h->addStretch(1);
        m_mouse->setFixedSize(92, 132);
        h->addWidget(m_mouse);
        h->addStretch(1);
        pv->addStretch(1);
        pv->addLayout(h);
        pv->addStretch(1);
    }
    v->addWidget(previewBox, 1);

    m_name = new QLabel(device->displayName(), this);
    m_name->setFont(Theme::font(15, QFont::DemiBold));
    m_name->setAttribute(Qt::WA_TransparentForMouseEvents);
    v->addWidget(m_name);

    auto *statusRow = new QHBoxLayout;
    statusRow->setSpacing(8);
    m_status = new QLabel(this);
    m_status->setProperty("role", QStringLiteral("secondary"));
    m_status->setFont(Theme::font(12));
    m_status->setAttribute(Qt::WA_TransparentForMouseEvents);
    statusRow->addWidget(m_status);
    statusRow->addStretch(1);
    m_battery = new QLabel(this);
    m_battery->setFont(Theme::font(12, QFont::DemiBold));
    m_battery->setAttribute(Qt::WA_TransparentForMouseEvents);
    statusRow->addWidget(m_battery);
    v->addLayout(statusRow);

    connect(device, &Device::statusChanged, this, &DeviceCard::refresh);
    connect(device, &Device::batteryChanged, this, &DeviceCard::refresh);
    refresh();
}

QString DeviceCard::statusText(const Device *device)
{
    switch (device->status()) {
    case Device::Status::Connecting:
        return tr("Connecting…");
    case Device::Status::Connected:
        return device->isWireless() ? tr("Connected · 2.4 GHz") : tr("Connected · USB");
    case Device::Status::Asleep:
        return tr("Receiver connected · mouse asleep");
    case Device::Status::Error:
        return tr("Cannot open device");
    }
    return QString();
}

QColor DeviceCard::statusColor(const Device *device)
{
    switch (device->status()) {
    case Device::Status::Connected:
        return Theme::Success;
    case Device::Status::Asleep:
    case Device::Status::Connecting:
        return Theme::Warning;
    case Device::Status::Error:
        return Theme::Danger;
    }
    return Theme::TextMuted;
}

void DeviceCard::refresh()
{
    m_status->setText(QStringLiteral("●  ") + statusText(m_device));
    m_status->setStyleSheet(QStringLiteral("color:%1;").arg(Theme::hex(statusColor(m_device))));
    if (m_device->hasBattery() && m_device->batteryLevel() >= 0) {
        const int lvl = m_device->batteryLevel();
        m_battery->setText(QStringLiteral("%1%2%").arg(m_device->isCharging() ? QStringLiteral("⚡ ") : QString()).arg(lvl));
        const QColor c = lvl <= 15 ? Theme::Danger : (lvl <= 30 ? Theme::Warning : Theme::TextSecondary);
        m_battery->setStyleSheet(QStringLiteral("color:%1;").arg(Theme::hex(c)));
        if (m_mouse)
            m_mouse->setBatteryLevel(lvl);
    } else {
        m_battery->clear();
        if (m_mouse)
            m_mouse->setBatteryLevel(-1);
    }
    update();
}

void DeviceCard::enterEvent(QEnterEvent *event)
{
    m_hover = true;
    update();
    QAbstractButton::enterEvent(event);
}

void DeviceCard::leaveEvent(QEvent *event)
{
    m_hover = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void DeviceCard::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    p.setBrush(m_hover || isDown() ? Theme::Bg3 : Theme::Bg2);
    p.setPen(QPen(m_hover ? Theme::Accent : Theme::Border, 1));
    p.drawRoundedRect(r, 12, 12);

    // subtle radial glow behind the device image
    QRadialGradient glow(QPointF(width() / 2.0, 90), 150);
    glow.setColorAt(0, QColor(255, 255, 255, m_hover ? 16 : 10));
    glow.setColorAt(1, QColor(255, 255, 255, 0));
    p.setPen(Qt::NoPen);
    p.setBrush(glow);
    p.drawRoundedRect(r.adjusted(1, 1, -1, -70), 12, 12);

    // arrow hint
    if (m_hover) {
        const QPixmap pm = Icons::pixmap(QStringLiteral("chevron-right"), Theme::Accent, 18, devicePixelRatioF());
        p.drawPixmap(width() - 18 - 16, 18, pm);
    }
}
