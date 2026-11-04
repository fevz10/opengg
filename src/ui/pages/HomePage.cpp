#include "ui/pages/HomePage.h"

#include "core/Icons.h"
#include "core/Theme.h"
#include "devices/Device.h"
#include "devices/DeviceManager.h"
#include "ui/pages/PageHelpers.h"
#include "ui/widgets/Card.h"
#include "ui/widgets/DeviceCard.h"
#include "ui/widgets/SectionLabel.h"

#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QTime>
#include <QVBoxLayout>

namespace {

class HeroCard : public QFrame
{
public:
    explicit HeroCard(QWidget *parent = nullptr)
        : QFrame(parent)
    {
        setMinimumHeight(170);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        QLinearGradient g(r.topLeft(), r.bottomRight());
        g.setColorAt(0.0, QColor(0x25, 0x1a, 0x14));
        g.setColorAt(0.55, QColor(0x1b, 0x1b, 0x1b));
        g.setColorAt(1.0, QColor(0x16, 0x16, 0x16));
        p.setPen(QPen(Theme::Border, 1));
        p.setBrush(g);
        p.drawRoundedRect(r, 14, 14);
        // decorative orange arcs on the right
        p.setPen(QPen(QColor(0xff, 0x52, 0x00, 70), 10));
        p.setBrush(Qt::NoBrush);
        p.drawArc(QRectF(r.right() - 150, r.top() - 60, 260, 260), 120 * 16, 110 * 16);
        p.setPen(QPen(QColor(0xff, 0x52, 0x00, 36), 10));
        p.drawArc(QRectF(r.right() - 120, r.top() - 30, 260, 260), 120 * 16, 110 * 16);
        p.setPen(QPen(QColor(0xff, 0x52, 0x00, 160), 4));
        p.drawArc(QRectF(r.right() - 182, r.top() - 92, 260, 260), 125 * 16, 100 * 16);
    }
};

QString userDisplayName()
{
    QString name = qEnvironmentVariable("USER");
    if (name.isEmpty())
        name = qEnvironmentVariable("USERNAME");
    if (name.isEmpty())
        name = QDir::home().dirName();
    return name;
}

} // namespace

HomePage::HomePage(const AppContext &ctx, QWidget *parent)
    : QWidget(parent)
    , m_ctx(ctx)
{
    QVBoxLayout *v = nullptr;
    QWidget *content = PageHelpers::makeContent(&v);

    // Hero
    auto *hero = new HeroCard(content);
    auto *hv = new QVBoxLayout(hero);
    hv->setContentsMargins(28, 26, 28, 26);
    hv->setSpacing(8);
    m_greeting = new QLabel(hero);
    m_greeting->setFont(Theme::titleFont(26));
    hv->addWidget(m_greeting);
    auto *sub = PageHelpers::secondary(tr("Your gear is ready. Lighting, CPI and button settings are restored automatically when a device connects."), hero);
    sub->setMaximumWidth(620);
    hv->addWidget(sub);
    hv->addSpacing(6);
    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);
    auto *engineBtn = new QPushButton(tr("Open Engine"), hero);
    engineBtn->setProperty("variant", QStringLiteral("primary"));
    engineBtn->setCursor(Qt::PointingHandCursor);
    connect(engineBtn, &QPushButton::clicked, this, &HomePage::openEngineRequested);
    auto *settingsBtn = new QPushButton(tr("Settings"), hero);
    settingsBtn->setCursor(Qt::PointingHandCursor);
    connect(settingsBtn, &QPushButton::clicked, this, &HomePage::openSettingsRequested);
    btnRow->addWidget(engineBtn);
    btnRow->addWidget(settingsBtn);
    btnRow->addStretch(1);
    hv->addLayout(btnRow);
    v->addWidget(hero);

    const int hour = QTime::currentTime().hour();
    QString greet = hour < 6 ? tr("Good night") : hour < 12 ? tr("Good morning") : hour < 18 ? tr("Good afternoon") : tr("Good evening");
    m_greeting->setText(QStringLiteral("%1, %2").arg(greet, userDisplayName()));

    // Gear
    v->addWidget(new SectionLabel(tr("Your gear"), content));
    auto *gearCard = new Card(content);
    gearCard->bodyLayout()->setSpacing(0);
    m_gearLayout = gearCard->bodyLayout();
    v->addWidget(gearCard);

    // Tips
    v->addWidget(new SectionLabel(tr("Good to know"), content));
    auto *tips = new QHBoxLayout;
    tips->setSpacing(16);
    tips->addWidget(makeTipCard(QStringLiteral("keyboard"), tr("Keep the app running"),
                                tr("The Apex 3 TKL has no on-board memory for custom lighting. Keep OpenGG running in the tray so your illumination survives reconnects and reboots.")));
    tips->addWidget(makeTipCard(QStringLiteral("mouse"), tr("Settings live on the mouse"),
                                tr("CPI presets, polling rate and button mapping are written to the Rival 3 Wireless Gen 2's on-board memory and keep working on any computer.")));
    tips->addWidget(makeTipCard(QStringLiteral("shield"), tr("Device permissions"),
                                tr("Access to the devices is granted through udev rules installed by the package. If a device shows as unavailable, check Settings › Permissions.")));
    v->addLayout(tips);
    v->addStretch(1);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(PageHelpers::makeScrollArea(content, this));

    connect(m_ctx.devices, &DeviceManager::devicesChanged, this, &HomePage::rebuildGear);
    rebuildGear();
}

QWidget *HomePage::makeTipCard(const QString &icon, const QString &title, const QString &text)
{
    auto *card = new Card(this);
    card->bodyLayout()->setSpacing(8);
    auto *row = new QHBoxLayout;
    row->setSpacing(12);
    auto *ic = new QLabel(card);
    ic->setPixmap(Icons::pixmap(icon, Theme::Accent, 20));
    ic->setFixedSize(36, 36);
    ic->setAlignment(Qt::AlignCenter);
    ic->setStyleSheet(QStringLiteral("background:%1; border-radius:18px;").arg(Theme::hex(Theme::Bg3)));
    row->addWidget(ic, 0, Qt::AlignTop);
    auto *t = new QLabel(title, card);
    t->setFont(Theme::font(14, QFont::DemiBold));
    t->setWordWrap(true);
    row->addWidget(t, 1, Qt::AlignVCenter);
    card->addLayout(row);
    auto *body = PageHelpers::secondary(text, card);
    body->setFont(Theme::font(12));
    card->addWidget(body);
    card->addStretch(1);
    return card;
}

QWidget *HomePage::makeGearRow(Device *device)
{
    auto *row = new QWidget(this);
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 10, 0, 10);
    h->setSpacing(14);

    auto *ic = new QLabel(row);
    ic->setPixmap(Icons::pixmap(device->kind() == Device::Kind::Keyboard ? QStringLiteral("keyboard") : QStringLiteral("mouse"),
                                Theme::Text, 20));
    ic->setFixedSize(40, 40);
    ic->setAlignment(Qt::AlignCenter);
    ic->setStyleSheet(QStringLiteral("background:%1; border-radius:10px;").arg(Theme::hex(Theme::Bg3)));
    h->addWidget(ic);

    auto *textBox = new QVBoxLayout;
    textBox->setSpacing(2);
    auto *name = new QLabel(device->modelName(), row);
    name->setFont(Theme::font(14, QFont::DemiBold));
    textBox->addWidget(name);
    auto *status = new QLabel(row);
    status->setFont(Theme::font(12));
    textBox->addWidget(status);
    h->addLayout(textBox, 1);

    auto *battery = new QLabel(row);
    battery->setFont(Theme::font(13, QFont::DemiBold));
    h->addWidget(battery);

    auto *configure = new QPushButton(tr("Configure"), row);
    configure->setProperty("variant", QStringLiteral("ghost"));
    configure->setCursor(Qt::PointingHandCursor);
    connect(configure, &QPushButton::clicked, this, [this, device]() { emit openDeviceRequested(device); });
    h->addWidget(configure);

    auto refresh = [device, status, battery]() {
        status->setText(QStringLiteral("●  ") + DeviceCard::statusText(device));
        status->setStyleSheet(QStringLiteral("color:%1;").arg(Theme::hex(DeviceCard::statusColor(device))));
        if (device->hasBattery() && device->batteryLevel() >= 0)
            battery->setText(tr("Battery %1%").arg(device->batteryLevel()));
        else
            battery->clear();
    };
    connect(device, &Device::statusChanged, row, refresh);
    connect(device, &Device::batteryChanged, row, refresh);
    refresh();
    return row;
}

void HomePage::rebuildGear()
{
    for (QWidget *w : m_gearRows) {
        m_gearLayout->removeWidget(w);
        w->deleteLater();
    }
    m_gearRows.clear();

    const QList<Device *> devices = m_ctx.devices->devices();
    if (devices.isEmpty()) {
        auto *empty = PageHelpers::secondary(tr("No supported device detected. Connect your Apex 3 TKL or the Rival 3 Wireless Gen 2 receiver."), this);
        empty->setContentsMargins(0, 6, 0, 6);
        m_gearLayout->addWidget(empty);
        m_gearRows.append(empty);
        return;
    }
    for (int i = 0; i < devices.size(); ++i) {
        if (i > 0) {
            QFrame *d = PageHelpers::divider(this);
            m_gearLayout->addWidget(d);
            m_gearRows.append(d);
        }
        QWidget *row = makeGearRow(devices.at(i));
        m_gearLayout->addWidget(row);
        m_gearRows.append(row);
    }
}
