#include "ui/pages/DevicePage.h"

#include "core/ConfigStore.h"
#include "core/Icons.h"
#include "core/Theme.h"
#include "devices/Apex3Tkl.h"
#include "devices/Device.h"
#include "devices/Rival3WirelessGen2.h"
#include "lighting/LightingEngine.h"
#include "ui/pages/KeyboardIlluminationTab.h"
#include "ui/pages/KeyboardSettingsTab.h"
#include "ui/pages/MouseButtonsTab.h"
#include "ui/pages/MouseSensitivityTab.h"
#include "ui/pages/MouseSettingsTab.h"
#include "ui/pages/PageHelpers.h"
#include "ui/widgets/Card.h"
#include "ui/widgets/ConfigList.h"
#include "ui/widgets/DeviceCard.h"
#include "ui/widgets/IconButton.h"
#include "ui/widgets/KeyboardPreview.h"
#include "ui/widgets/MousePreview.h"
#include "ui/widgets/NavTab.h"
#include "ui/widgets/Toast.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>

DevicePage::DevicePage(const AppContext &ctx, Device *device, QWidget *parent)
    : QWidget(parent)
    , m_ctx(ctx)
    , m_device(device)
    , m_tabGroup(new QButtonGroup(this))
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(36, 22, 36, 0);
    outer->setSpacing(18);

    // ---- header -------------------------------------------------------------
    auto *header = new QHBoxLayout;
    header->setSpacing(10);
    auto *back = new IconButton(QStringLiteral("chevron-left"), this);
    back->setToolTip(tr("Back to Engine"));
    connect(back, &QAbstractButton::clicked, this, &DevicePage::backRequested);
    header->addWidget(back);
    auto *crumb = new QLabel(tr("Engine"), this);
    crumb->setFont(Theme::font(15));
    crumb->setProperty("role", QStringLiteral("muted"));
    header->addWidget(crumb);
    auto *sep = new QLabel(QStringLiteral("›"), this);
    sep->setProperty("role", QStringLiteral("muted"));
    sep->setFont(Theme::font(15));
    header->addWidget(sep);
    auto *name = new QLabel(device->displayName(), this);
    name->setFont(Theme::titleFont(20));
    header->addWidget(name);
    header->addStretch(1);
    outer->addLayout(header);

    // ---- body ----------------------------------------------------------------
    auto *body = new QHBoxLayout;
    body->setSpacing(24);

    // Left column
    auto *left = new QWidget(this);
    left->setFixedWidth(300);
    auto *lv = new QVBoxLayout(left);
    lv->setContentsMargins(0, 0, 0, 0);
    lv->setSpacing(16);

    auto *previewCard = new Card(left);
    previewCard->bodyLayout()->setContentsMargins(0, 0, 0, 0);
    if (device->kind() == Device::Kind::Keyboard) {
        auto *kb = new KeyboardPreview(previewCard);
        kb->setInteractive(false);
        if (auto *apex = qobject_cast<Apex3Tkl *>(device)) {
            connect(apex->engine(), &LightingEngine::previewFrame, kb, &KeyboardPreview::setFrame);
            kb->setFrame(apex->engine()->lastPreviewFrame());
        }
        previewCard->addWidget(kb);
    } else {
        auto *mp = new MousePreview(previewCard);
        mp->setShowCallouts(false);
        mp->setFixedSize(120, 172);
        auto *h = new QHBoxLayout;
        h->addStretch(1);
        h->addWidget(mp);
        h->addStretch(1);
        previewCard->addLayout(h);
        connect(device, &Device::batteryChanged, mp, [mp, device]() { mp->setBatteryLevel(device->batteryLevel()); });
        mp->setBatteryLevel(device->batteryLevel());
    }
    auto *model = new QLabel(device->modelName(), previewCard);
    model->setFont(Theme::font(14, QFont::DemiBold));
    model->setAlignment(Qt::AlignCenter);
    model->setWordWrap(true);
    previewCard->addWidget(model);
    m_statusLabel = new QLabel(previewCard);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setFont(Theme::font(12));
    previewCard->addWidget(m_statusLabel);
    m_batteryLabel = new QLabel(previewCard);
    m_batteryLabel->setAlignment(Qt::AlignCenter);
    m_batteryLabel->setFont(Theme::font(12, QFont::DemiBold));
    m_batteryLabel->setProperty("role", QStringLiteral("secondary"));
    previewCard->addWidget(m_batteryLabel);
    lv->addWidget(previewCard);

    m_configList = new ConfigList(left);
    lv->addWidget(m_configList);
    lv->addStretch(1);
    body->addWidget(left, 0, Qt::AlignTop);

    // Right column: tabs
    auto *right = new QWidget(this);
    auto *rv = new QVBoxLayout(right);
    rv->setContentsMargins(0, 0, 0, 0);
    rv->setSpacing(0);

    auto *tabBarWidget = new QWidget(right);
    m_tabBar = new QHBoxLayout(tabBarWidget);
    m_tabBar->setContentsMargins(0, 0, 0, 0);
    m_tabBar->setSpacing(4);
    m_tabBar->addStretch(1);
    rv->addWidget(tabBarWidget);
    rv->addWidget(PageHelpers::divider(right));

    m_tabStack = new QStackedWidget(right);
    m_tabStack->setObjectName(QStringLiteral("DeviceTabStack"));
    auto *scroll = new QScrollArea(right);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *stackHolder = new QWidget;
    auto *sh = new QVBoxLayout(stackHolder);
    sh->setContentsMargins(0, 20, 0, 36);
    sh->addWidget(m_tabStack);
    sh->addStretch(1);
    scroll->setWidget(stackHolder);
    rv->addWidget(scroll, 1);
    body->addWidget(right, 1);
    outer->addLayout(body, 1);

    m_tabGroup->setExclusive(true);
    connect(m_tabGroup, &QButtonGroup::idClicked, this, [this](int id) { m_tabStack->setCurrentIndex(id); });

    // ---- tabs per device type ------------------------------------------------
    if (auto *apex = qobject_cast<Apex3Tkl *>(device)) {
        addTab(tr("Illumination"), new KeyboardIlluminationTab(apex, this));
        addTab(tr("Settings"), new KeyboardSettingsTab(apex, this));
    } else if (auto *rival = qobject_cast<Rival3WirelessGen2 *>(device)) {
        addTab(tr("Buttons"), new MouseButtonsTab(rival, this));
        addTab(tr("Sensitivity (CPI)"), new MouseSensitivityTab(rival, this));
        addTab(tr("Settings"), new MouseSettingsTab(rival, m_ctx, this));
    }
    if (!m_tabButtons.isEmpty())
        m_tabButtons.first()->setChecked(true);

    // ---- configs ---------------------------------------------------------------
    ConfigStore *store = m_ctx.store;
    const QString key = device->key();
    store->ensureDefaultConfig(key, device->defaultConfig(), tr("Default config"));

    connect(m_configList, &ConfigList::configSelected, this, [this, store, key](const QString &id) {
        store->setActiveConfigId(key, id);
    });
    connect(m_configList, &ConfigList::addRequested, this, [this, store, key]() {
        bool ok = false;
        const QString name = QInputDialog::getText(this, tr("New config"), tr("Config name:"), QLineEdit::Normal,
                                                   tr("Config %1").arg(store->configs(key).size() + 1), &ok);
        if (!ok || name.trimmed().isEmpty())
            return;
        const ConfigStore::NamedConfig c = store->addConfig(key, name.trimmed(), m_device->defaultConfig());
        store->setActiveConfigId(key, c.id);
    });
    connect(m_configList, &ConfigList::renameRequested, this, [this, store, key](const QString &id) {
        bool ok = false;
        const QString name = QInputDialog::getText(this, tr("Rename config"), tr("Config name:"), QLineEdit::Normal,
                                                   store->config(key, id).name, &ok);
        if (ok && !name.trimmed().isEmpty())
            store->renameConfig(key, id, name.trimmed());
    });
    connect(m_configList, &ConfigList::duplicateRequested, this, [this, store, key](const QString &id) {
        const ConfigStore::NamedConfig src = store->config(key, id);
        const ConfigStore::NamedConfig c = store->addConfig(key, tr("%1 (copy)").arg(src.name), src.data);
        store->setActiveConfigId(key, c.id);
    });
    connect(m_configList, &ConfigList::deleteRequested, this, [this, store, key](const QString &id) {
        const ConfigStore::NamedConfig c = store->config(key, id);
        const auto answer = QMessageBox::question(this, tr("Delete config"), tr("Delete “%1”? This cannot be undone.").arg(c.name),
                                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer == QMessageBox::Yes)
            store->removeConfig(key, id);
    });
    connect(store, &ConfigStore::configsChanged, this, [this, key](const QString &k) {
        if (k == key)
            reloadConfigs();
    });
    connect(store, &ConfigStore::activeConfigChanged, this, [this, key](const QString &k, const QString &) {
        if (k != key)
            return;
        reloadConfigs();
        loadActiveIntoTabs();
    });
    // Config data may change from elsewhere (brightness keys on the keyboard,
    // another window): keep the tabs in sync.
    connect(store, &ConfigStore::configDataChanged, this, [this, key](const QString &k, const QString &id) {
        if (k == key && id == m_ctx.store->activeConfigId(key))
            loadActiveIntoTabs();
    });

    connect(device, &Device::statusChanged, this, &DevicePage::refreshStatus);
    connect(device, &Device::batteryChanged, this, &DevicePage::refreshStatus);

    reloadConfigs();
    loadActiveIntoTabs();
    refreshStatus();
}

void DevicePage::addTab(const QString &title, ConfigTab *tab)
{
    auto *button = new NavTab(title, this);
    button->setCompact(true);
    const int index = m_tabs.size();
    m_tabGroup->addButton(button, index);
    m_tabBar->insertWidget(index, button);
    m_tabButtons.append(button);
    m_tabs.append(tab);
    m_tabStack->addWidget(tab);
    connect(tab, &ConfigTab::configEdited, this, &DevicePage::onConfigEdited);
}

void DevicePage::setCurrentTab(int index)
{
    if (index < 0 || index >= m_tabButtons.size())
        return;
    m_tabButtons.at(index)->setChecked(true);
    m_tabStack->setCurrentIndex(index);
}

void DevicePage::reloadConfigs()
{
    const QString key = m_device->key();
    m_configList->setConfigs(m_ctx.store->configs(key), m_ctx.store->activeConfigId(key));
}

void DevicePage::loadActiveIntoTabs()
{
    m_loading = true;
    const QJsonObject data = m_ctx.store->activeConfig(m_device->key()).data;
    for (ConfigTab *t : m_tabs)
        t->loadConfig(data);
    m_loading = false;
}

void DevicePage::onConfigEdited(const QJsonObject &data)
{
    if (m_loading)
        return;
    const QString key = m_device->key();
    const QString id = m_ctx.store->activeConfigId(key);
    // The store emits configDataChanged -> the device controller applies it
    // and loadActiveIntoTabs() keeps every tab in sync.
    m_ctx.store->updateConfigData(key, id, data);
}

void DevicePage::refreshStatus()
{
    m_statusLabel->setText(QStringLiteral("●  ") + DeviceCard::statusText(m_device));
    m_statusLabel->setStyleSheet(QStringLiteral("color:%1;").arg(Theme::hex(DeviceCard::statusColor(m_device))));
    if (m_device->hasBattery()) {
        if (m_device->batteryLevel() >= 0)
            m_batteryLabel->setText(m_device->isCharging() ? tr("Battery %1% · charging").arg(m_device->batteryLevel())
                                                           : tr("Battery %1%").arg(m_device->batteryLevel()));
        else
            m_batteryLabel->setText(tr("Battery level unknown"));
        m_batteryLabel->setVisible(true);
    } else {
        m_batteryLabel->setVisible(false);
    }
}
