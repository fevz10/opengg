#include "ui/pages/EnginePage.h"

#include "core/Icons.h"
#include "core/Theme.h"
#include "devices/Device.h"
#include "devices/DeviceManager.h"
#include "ui/pages/DevicePage.h"
#include "ui/pages/PageHelpers.h"
#include "ui/widgets/Card.h"
#include "ui/widgets/DeviceCard.h"
#include "ui/widgets/EmptyState.h"
#include "ui/widgets/FlowLayout.h"
#include "ui/widgets/SectionLabel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

EnginePage::EnginePage(const AppContext &ctx, QWidget *parent)
    : QWidget(parent)
    , m_ctx(ctx)
    , m_stack(new QStackedWidget(this))
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(m_stack);

    // ---- grid page --------------------------------------------------------
    QVBoxLayout *v = nullptr;
    QWidget *content = PageHelpers::makeContent(&v, 1400);

    auto *header = new QHBoxLayout;
    header->addWidget(PageHelpers::title(tr("Engine"), content));
    if (m_ctx.demo) {
        auto *demo = new QLabel(tr("Demo mode"), content);
        demo->setProperty("role", QStringLiteral("badge-accent"));
        demo->setToolTip(tr("Simulated devices — nothing is sent to real hardware."));
        header->addWidget(demo, 0, Qt::AlignVCenter);
    }
    header->addStretch(1);
    auto *rescan = new QPushButton(tr("Rescan"), content);
    rescan->setProperty("variant", QStringLiteral("ghost"));
    rescan->setIcon(Icons::icon(QStringLiteral("refresh"), Theme::TextSecondary, 16));
    rescan->setCursor(Qt::PointingHandCursor);
    connect(rescan, &QPushButton::clicked, this, [this]() { m_ctx.devices->rescan(); });
    header->addWidget(rescan);
    v->addLayout(header);

    v->addWidget(new SectionLabel(tr("Gear"), content));

    m_gearContainer = new QWidget(content);
    m_gearFlow = new FlowLayout(m_gearContainer, 0, 16, 16);
    v->addWidget(m_gearContainer);

    m_empty = new EmptyState(QStringLiteral("usb"), tr("No gear detected"),
                             tr("Connect your SteelSeries Apex 3 TKL or plug in the Rival 3 Wireless Gen 2 receiver. "
                                "If a device is connected but not listed, the udev permission rules may be missing."),
                             content);
    QPushButton *perm = m_empty->addAction(tr("Permissions help"));
    connect(perm, &QPushButton::clicked, this, &EnginePage::openSettingsRequested);
    QPushButton *rescan2 = m_empty->addAction(tr("Rescan"), true);
    connect(rescan2, &QPushButton::clicked, this, [this]() { m_ctx.devices->rescan(); });
    m_empty->setVisible(false);
    v->addWidget(m_empty);

    m_unsupportedBox = new QWidget(content);
    auto *ub = new QVBoxLayout(m_unsupportedBox);
    ub->setContentsMargins(0, 0, 0, 0);
    ub->setSpacing(10);
    m_unsupportedBox->setVisible(false);
    v->addWidget(m_unsupportedBox);

    v->addSpacing(8);
    v->addWidget(new SectionLabel(tr("Apps"), content));
    auto *apps = new QHBoxLayout;
    apps->setSpacing(16);
    apps->addWidget(makeAppCard(QStringLiteral("sync"), QStringLiteral("PrismSync"),
                                tr("Synchronises illumination across devices. Needs at least two illuminated devices.")));
    apps->addWidget(makeAppCard(QStringLiteral("zap"), QStringLiteral("GameSense"),
                                tr("In-game events on your devices. Requires the Windows GameSense SDK.")));
    apps->addWidget(makeAppCard(QStringLiteral("activity"), tr("Audio Visualizer"),
                                tr("Illumination reacting to system audio. Not available on Linux yet.")));
    v->addLayout(apps);
    v->addStretch(1);

    m_gridPage = PageHelpers::makeScrollArea(content, this);
    m_stack->addWidget(m_gridPage);

    connect(m_ctx.devices, &DeviceManager::devicesChanged, this, &EnginePage::rebuildGrid);
    connect(m_ctx.devices, &DeviceManager::deviceRemoved, this, [this](Device *device) {
        if (auto *page = qobject_cast<DevicePage *>(m_devicePage); page && page->device() == device)
            showGrid();
    });
    rebuildGrid();
}

QWidget *EnginePage::makeAppCard(const QString &icon, const QString &name, const QString &description)
{
    auto *card = new Card(this);
    card->bodyLayout()->setSpacing(8);
    auto *row = new QHBoxLayout;
    row->setSpacing(12);
    auto *ic = new QLabel(card);
    ic->setPixmap(Icons::pixmap(icon, Theme::TextSecondary, 20));
    ic->setFixedSize(40, 40);
    ic->setAlignment(Qt::AlignCenter);
    ic->setStyleSheet(QStringLiteral("background:%1; border-radius:10px;").arg(Theme::hex(Theme::Bg3)));
    row->addWidget(ic);
    auto *n = new QLabel(name, card);
    n->setFont(Theme::font(14, QFont::DemiBold));
    row->addWidget(n, 1);
    auto *badge = new QLabel(tr("Windows only"), card);
    badge->setProperty("role", QStringLiteral("badge"));
    row->addWidget(badge, 0, Qt::AlignTop);
    card->addLayout(row);
    auto *d = PageHelpers::secondary(description, card);
    d->setFont(Theme::font(12));
    card->addWidget(d);
    card->addStretch(1);
    return card;
}

QWidget *EnginePage::makeUnsupportedCard(const QString &product, quint16 pid)
{
    auto *card = new Card(this);
    auto *row = new QHBoxLayout;
    row->setSpacing(12);
    auto *ic = new QLabel(card);
    ic->setPixmap(Icons::pixmap(QStringLiteral("info"), Theme::Warning, 18));
    row->addWidget(ic, 0, Qt::AlignTop);
    auto *text = new QLabel(tr("<b>%1</b> (1038:%2) was found but is not supported by this app yet.")
                                .arg(product.isEmpty() ? tr("Unknown SteelSeries device") : product.toHtmlEscaped(),
                                     QStringLiteral("%1").arg(pid, 4, 16, QLatin1Char('0'))),
                            card);
    text->setWordWrap(true);
    text->setProperty("role", QStringLiteral("secondary"));
    row->addWidget(text, 1);
    card->addLayout(row);
    return card;
}

void EnginePage::rebuildGrid()
{
    for (QWidget *w : m_cards) {
        m_gearFlow->removeWidget(w);
        w->deleteLater();
    }
    m_cards.clear();

    const QList<Device *> devices = m_ctx.devices->devices();
    for (Device *d : devices) {
        auto *card = new DeviceCard(d, m_gearContainer);
        connect(card, &QAbstractButton::clicked, this, [this, d]() { openDevice(d); });
        m_gearFlow->addWidget(card);
        m_cards.append(card);
    }
    m_gearContainer->setVisible(!devices.isEmpty());
    m_empty->setVisible(devices.isEmpty());

    // Unsupported devices
    QLayout *ul = m_unsupportedBox->layout();
    while (QLayoutItem *item = ul->takeAt(0)) {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }
    const QList<HidDeviceInfo> unsupported = m_ctx.devices->unsupportedDevices();
    for (const HidDeviceInfo &u : unsupported)
        ul->addWidget(makeUnsupportedCard(u.product, u.productId));
    m_unsupportedBox->setVisible(!unsupported.isEmpty());
}

void EnginePage::openDevice(Device *device)
{
    if (!device)
        return;
    if (m_devicePage) {
        m_stack->removeWidget(m_devicePage);
        m_devicePage->setParent(nullptr);
        m_devicePage->deleteLater();
        m_devicePage = nullptr;
    }
    auto *page = new DevicePage(m_ctx, device, this);
    connect(page, &DevicePage::backRequested, this, &EnginePage::showGrid);
    m_devicePage = page;
    m_stack->addWidget(page);
    m_stack->setCurrentWidget(page);
}

void EnginePage::showGrid()
{
    m_stack->setCurrentWidget(m_gridPage);
    if (m_devicePage) {
        m_stack->removeWidget(m_devicePage);
        m_devicePage->setParent(nullptr);
        m_devicePage->deleteLater();
        m_devicePage = nullptr;
    }
}

bool EnginePage::isShowingDevice() const
{
    return m_devicePage && m_stack->currentWidget() == m_devicePage;
}

DevicePage *EnginePage::devicePage() const
{
    return isShowingDevice() ? qobject_cast<DevicePage *>(m_devicePage) : nullptr;
}
