#include "ui/pages/SettingsPage.h"

#include "core/Autostart.h"
#include "core/ConfigStore.h"
#include "core/Icons.h"
#include "core/Theme.h"
#include "devices/Device.h"
#include "devices/DeviceManager.h"
#include "hid/HidTransport.h"
#include "ui/pages/PageHelpers.h"
#include "ui/widgets/Card.h"
#include "ui/widgets/SectionLabel.h"
#include "ui/widgets/Toast.h"
#include "ui/widgets/ToggleSwitch.h"

#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QUrl>
#include <QVBoxLayout>

namespace {
const char *kRulesInstallPath = "/etc/udev/rules.d/70-opengg.rules";
const char *kRulesPackagePath = "/usr/lib/udev/rules.d/70-opengg.rules";
} // namespace

SettingsPage::SettingsPage(const AppContext &ctx, HidTransport *transport, QWidget *parent)
    : QWidget(parent)
    , m_ctx(ctx)
{
    QVBoxLayout *v = nullptr;
    QWidget *content = PageHelpers::makeContent(&v, 980);
    v->addWidget(PageHelpers::title(tr("Settings"), content));

    // ---- General ------------------------------------------------------------
    v->addWidget(new SectionLabel(tr("General"), content));
    auto *general = new Card(content);

    m_language = new QComboBox(general);
    m_language->addItem(tr("System default"), QStringLiteral("system"));
    m_language->addItem(QStringLiteral("English"), QStringLiteral("en"));
    m_language->addItem(QStringLiteral("Türkçe"), QStringLiteral("tr"));
    m_language->setFixedWidth(200);
    m_language->setCursor(Qt::PointingHandCursor);
    general->addRow(tr("Language"), m_language, tr("Takes effect after restarting the app."));
    m_restartHint = PageHelpers::secondary(tr("Restart the app to apply the new language."), general);
    m_restartHint->setProperty("role", QStringLiteral("accent"));
    m_restartHint->setVisible(false);
    general->addWidget(m_restartHint);
    general->addWidget(PageHelpers::divider(general));

    m_autostart = new ToggleSwitch(general);
    general->addRow(tr("Launch at login"), m_autostart, tr("Recommended: the Apex 3 TKL only keeps custom lighting while the app runs."));
    m_startMinimized = new ToggleSwitch(general);
    general->addRow(tr("Start minimized to the tray"), m_startMinimized);
    m_closeToTray = new ToggleSwitch(general);
    const bool trayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    general->addRow(tr("Keep running in the tray when the window is closed"), m_closeToTray,
                    trayAvailable ? QString()
                                  : tr("No system tray was found in this desktop session, so closing the window quits the app "
                                       "(GNOME needs the AppIndicator extension)."));
    m_closeToTray->setEnabled(trayAvailable);
    m_notifications = new ToggleSwitch(general);
    general->addRow(tr("Show notifications"), m_notifications, tr("Device connected / disconnected and low battery notices."));
    v->addWidget(general);

    // ---- Permissions ----------------------------------------------------------
    v->addWidget(new SectionLabel(tr("Devices & permissions"), content));
    auto *perm = new Card(content);
    auto *permRow = new QHBoxLayout;
    permRow->setSpacing(12);
    m_permissionIcon = new QLabel(perm);
    m_permissionIcon->setFixedSize(20, 20);
    permRow->addWidget(m_permissionIcon, 0, Qt::AlignTop);
    m_permissionStatus = PageHelpers::secondary(QString(), perm);
    permRow->addWidget(m_permissionStatus, 1);
    perm->addLayout(permRow);
    auto *permButtons = new QHBoxLayout;
    permButtons->setSpacing(10);
    m_installRules = new QPushButton(tr("Install udev rules…"), perm);
    m_installRules->setCursor(Qt::PointingHandCursor);
    connect(m_installRules, &QPushButton::clicked, this, &SettingsPage::installUdevRules);
    permButtons->addWidget(m_installRules);
    auto *rescan = new QPushButton(tr("Rescan devices"), perm);
    rescan->setProperty("variant", QStringLiteral("ghost"));
    rescan->setCursor(Qt::PointingHandCursor);
    connect(rescan, &QPushButton::clicked, this, [this]() { m_ctx.devices->rescan(); });
    permButtons->addWidget(rescan);
    permButtons->addStretch(1);
    perm->addLayout(permButtons);
    auto *supported = PageHelpers::secondary(tr("Supported hardware: SteelSeries Apex 3 TKL (1038:1622) and Rival 3 Wireless Gen 2 receiver (1038:1872)."), perm);
    supported->setFont(Theme::font(12));
    perm->addWidget(supported);
    v->addWidget(perm);

    // ---- Diagnostics ---------------------------------------------------------
    v->addWidget(new SectionLabel(tr("Diagnostics"), content));
    auto *diag = new Card(tr("HID traffic log"), tr("The last commands exchanged with the devices. Useful when reporting a problem."), content);
    m_log = new QPlainTextEdit(diag);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(400);
    m_log->setFixedHeight(180);
    QFont mono = QFont(QStringLiteral("monospace"));
    mono.setStyleHint(QFont::Monospace);
    mono.setPixelSize(11);
    m_log->setFont(mono);
    m_log->setStyleSheet(QStringLiteral("QPlainTextEdit{background:%1;border:1px solid %2;border-radius:6px;color:%3;padding:6px;}")
                             .arg(Theme::hex(Theme::Bg0), Theme::hex(Theme::Border), Theme::hex(Theme::TextSecondary)));
    diag->addWidget(m_log);
    auto *diagRow = new QHBoxLayout;
    auto *openConfig = new QPushButton(tr("Open config folder"), diag);
    openConfig->setProperty("variant", QStringLiteral("ghost"));
    openConfig->setCursor(Qt::PointingHandCursor);
    connect(openConfig, &QPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_ctx.store->filePath()).absolutePath()));
    });
    diagRow->addWidget(openConfig);
    auto *clearLog = new QPushButton(tr("Clear log"), diag);
    clearLog->setProperty("variant", QStringLiteral("ghost"));
    clearLog->setCursor(Qt::PointingHandCursor);
    connect(clearLog, &QPushButton::clicked, m_log, &QPlainTextEdit::clear);
    diagRow->addWidget(clearLog);
    diagRow->addStretch(1);
    if (m_ctx.demo) {
        auto *demo = new QLabel(tr("Demo mode — simulated devices"), diag);
        demo->setProperty("role", QStringLiteral("badge-accent"));
        diagRow->addWidget(demo);
    }
    diag->addLayout(diagRow);
    v->addWidget(diag);

    // ---- About ---------------------------------------------------------------
    v->addWidget(new SectionLabel(tr("About"), content));
    auto *about = new Card(content);
    auto *aboutRow = new QHBoxLayout;
    aboutRow->setSpacing(16);
    auto *logo = new QLabel(about);
    logo->setPixmap(Icons::appIcon(56));
    aboutRow->addWidget(logo, 0, Qt::AlignTop);
    auto *aboutText = new QLabel(about);
    aboutText->setWordWrap(true);
    aboutText->setOpenExternalLinks(true);
    aboutText->setTextFormat(Qt::RichText);
    aboutText->setText(tr("<b style='font-size:15px'>OpenGG</b> &nbsp;<span style='color:%1'>v%2</span><br/>"
                          "<span style='color:%3'>An unofficial, community made alternative to SteelSeries GG for Linux, supporting the Apex 3 TKL "
                          "and the Rival 3 Wireless Gen 2. Built with Qt %4 and hidapi.<br/><br/>"
                          "Not affiliated with, endorsed by or supported by SteelSeries. SteelSeries, GG, Apex and Rival are trademarks of SteelSeries ApS.<br/>"
                          "Protocol knowledge courtesy of the rivalcfg and OpenRGB projects. Icons based on Feather Icons (MIT).</span>")
                           .arg(Theme::hex(Theme::TextMuted), QStringLiteral(SSGG_VERSION), Theme::hex(Theme::TextSecondary), QStringLiteral(QT_VERSION_STR)));
    aboutRow->addWidget(aboutText, 1);
    about->addLayout(aboutRow);
    v->addWidget(about);
    v->addStretch(1);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(PageHelpers::makeScrollArea(content, this));

    // ---- load values ------------------------------------------------------------
    ConfigStore *store = m_ctx.store;
    const QString lang = store->value(QStringLiteral("language"), QStringLiteral("system")).toString();
    m_language->setCurrentIndex(qMax(0, m_language->findData(lang)));
    m_autostart->setChecked(Autostart::isEnabled());
    m_startMinimized->setChecked(store->value(QStringLiteral("startMinimized"), true).toBool());
    m_closeToTray->setChecked(store->value(QStringLiteral("closeToTray"), true).toBool());
    m_notifications->setChecked(store->value(QStringLiteral("notifications"), true).toBool());

    connect(m_language, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, store](int) {
        store->setValue(QStringLiteral("language"), m_language->currentData().toString());
        m_restartHint->setVisible(true);
    });
    connect(m_autostart, &QAbstractButton::toggled, this, [this, store](bool on) {
        if (!Autostart::setEnabled(on, m_startMinimized->isChecked()) && m_ctx.toasts)
            m_ctx.toasts->show(tr("Could not update the autostart entry"), Toast::Error);
        store->setValue(QStringLiteral("autostart"), on);
    });
    connect(m_startMinimized, &QAbstractButton::toggled, this, [this, store](bool on) {
        store->setValue(QStringLiteral("startMinimized"), on);
        if (m_autostart->isChecked())
            Autostart::setEnabled(true, on);
    });
    connect(m_closeToTray, &QAbstractButton::toggled, this, [store](bool on) { store->setValue(QStringLiteral("closeToTray"), on); });
    connect(m_notifications, &QAbstractButton::toggled, this, [store](bool on) { store->setValue(QStringLiteral("notifications"), on); });

    connect(m_ctx.devices, &DeviceManager::devicesChanged, this, &SettingsPage::refreshPermissions);
    connect(m_ctx.devices, &DeviceManager::deviceAdded, this, [this](Device *d) {
        connect(d, &Device::statusChanged, this, &SettingsPage::refreshPermissions);
    });
    for (Device *d : m_ctx.devices->devices())
        connect(d, &Device::statusChanged, this, &SettingsPage::refreshPermissions);
    if (transport)
        connect(transport, &HidTransport::logLine, this, &SettingsPage::appendLog);
    refreshPermissions();
}

void SettingsPage::appendLog(const QString &line)
{
    m_log->appendPlainText(line);
}

void SettingsPage::refreshPermissions()
{
    const bool rulesInstalled = QFile::exists(QString::fromLatin1(kRulesInstallPath)) || QFile::exists(QString::fromLatin1(kRulesPackagePath));
    bool anyError = false;
    for (Device *d : m_ctx.devices->devices())
        anyError = anyError || d->status() == Device::Status::Error;

    QString text;
    QColor color;
    QString icon;
    if (anyError) {
        icon = QStringLiteral("alert-triangle");
        color = Theme::Danger;
        text = tr("A device could not be opened. This is usually a permission problem: install the udev rules below, then unplug and replug the device.");
    } else if (!rulesInstalled && !m_ctx.demo) {
        icon = QStringLiteral("info");
        color = Theme::Warning;
        text = tr("The udev rules are not installed. Devices may only be accessible as root until you install them.");
    } else {
        icon = QStringLiteral("check-circle");
        color = Theme::Success;
        text = m_ctx.demo ? tr("Demo mode: no real hardware is accessed.")
                          : tr("Device access rules are installed. Supported devices can be configured without root privileges.");
    }
    m_permissionIcon->setPixmap(Icons::pixmap(icon, color, 20));
    m_permissionStatus->setText(text);
}

void SettingsPage::installUdevRules()
{
    // Write the bundled rules file to a temporary location and copy it with
    // elevated privileges through pkexec (PolicyKit).
    const QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString tmpFile = tmpDir + QStringLiteral("/70-opengg.rules");
    QFile::remove(tmpFile);
    if (!QFile::copy(QStringLiteral(":/udev/70-opengg.rules"), tmpFile)) {
        if (m_ctx.toasts)
            m_ctx.toasts->show(tr("Could not prepare the rules file"), Toast::Error);
        return;
    }
    QFile::setPermissions(tmpFile, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther);

    const QString script = QStringLiteral("install -m 0644 '%1' '%2' && udevadm control --reload-rules && "
                                          "udevadm trigger --subsystem-match=hidraw --action=add && "
                                          "udevadm trigger --subsystem-match=usb --attr-match=idVendor=1038 --action=add")
                               .arg(tmpFile, QString::fromLatin1(kRulesInstallPath));

    auto *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, proc, tmpFile](int code, QProcess::ExitStatus) {
        QFile::remove(tmpFile);
        if (code == 0) {
            if (m_ctx.toasts)
                m_ctx.toasts->show(tr("udev rules installed. Replug the devices if they are still unavailable."), Toast::Success, 6000);
            m_ctx.devices->rescan();
        } else if (code == 126 || code == 127) {
            if (m_ctx.toasts)
                m_ctx.toasts->show(tr("Authorisation cancelled"), Toast::Warning);
        } else {
            const QString err = QString::fromUtf8(proc->readAllStandardError()).trimmed();
            if (m_ctx.toasts)
                m_ctx.toasts->show(tr("Installing the rules failed: %1").arg(err.isEmpty() ? tr("exit code %1").arg(code) : err), Toast::Error, 8000);
        }
        refreshPermissions();
        proc->deleteLater();
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError) {
        if (m_ctx.toasts)
            m_ctx.toasts->show(tr("pkexec is not available. Run scripts/install-udev-rules.sh instead."), Toast::Error, 8000);
        proc->deleteLater();
    });
    proc->start(QStringLiteral("pkexec"), {QStringLiteral("sh"), QStringLiteral("-c"), script});
}
