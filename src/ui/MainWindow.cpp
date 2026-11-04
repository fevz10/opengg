#include "ui/MainWindow.h"

#include "core/ConfigStore.h"
#include "core/Icons.h"
#include "core/Theme.h"
#include "devices/Device.h"
#include "devices/DeviceManager.h"
#include "ui/DeviceController.h"
#include "ui/pages/EnginePage.h"
#include "ui/pages/HomePage.h"
#include "ui/pages/PlaceholderPage.h"
#include "ui/pages/SettingsPage.h"
#include "ui/widgets/Toast.h"
#include "ui/widgets/TopBar.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QMenu>
#include <QShortcut>
#include <QStackedWidget>
#include <QSystemTrayIcon>
#include <QVBoxLayout>

MainWindow::MainWindow(const AppContext &ctx, HidTransport *transport, QWidget *parent)
    : QMainWindow(parent)
    , m_ctx(ctx)
{
    setWindowTitle(QStringLiteral("OpenGG"));
    setWindowIcon(Icons::appIconSet());
    setMinimumSize(1024, 680);
    resize(1280, 800);

    m_toasts = new ToastHost(this);
    m_ctx.toasts = m_toasts;

    auto *central = new QWidget(this);
    central->setAutoFillBackground(true);
    QPalette pal = central->palette();
    pal.setColor(QPalette::Window, Theme::Bg1);
    central->setPalette(pal);
    auto *v = new QVBoxLayout(central);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    m_topBar = new TopBar(central);
    m_topBar->addTab(tr("Home"));
    m_topBar->addTab(tr("Engine"));
    m_topBar->addTab(tr("Sonar"));
    m_topBar->addTab(tr("Moments"));
    QString user = qEnvironmentVariable("USER");
    if (user.isEmpty())
        user = QDir::home().dirName();
    m_topBar->setUserName(user);
    v->addWidget(m_topBar);

    m_stack = new QStackedWidget(central);
    m_home = new HomePage(m_ctx, m_stack);
    m_engine = new EnginePage(m_ctx, m_stack);
    auto *sonar = new PlaceholderPage(
        QStringLiteral("headphones"), tr("Sonar is not available on Linux"),
        tr("Sonar is SteelSeries' Windows-only audio suite (virtual mixer, ClearCast AI noise cancelling, parametric EQ). "
           "On Linux the same ground is covered by PipeWire based tools."),
        {{tr("EasyEffects (EQ & noise reduction)"), QStringLiteral("https://github.com/wwmm/easyeffects")},
         {tr("Helvum (PipeWire patchbay)"), QStringLiteral("https://gitlab.freedesktop.org/pipewire/helvum")}},
        m_stack);
    auto *moments = new PlaceholderPage(
        QStringLiteral("film"), tr("Moments is not available on Linux"),
        tr("Moments captures and trims gameplay clips on Windows. For Linux, these open source tools do the job well."),
        {{tr("OBS Studio"), QStringLiteral("https://obsproject.com")},
         {tr("GPU Screen Recorder"), QStringLiteral("https://git.dec05eba.com/gpu-screen-recorder/about/")}},
        m_stack);
    m_settings = new SettingsPage(m_ctx, transport, m_stack);

    m_stack->addWidget(m_home);
    m_stack->addWidget(m_engine);
    m_stack->addWidget(sonar);
    m_stack->addWidget(moments);
    m_stack->addWidget(m_settings);
    v->addWidget(m_stack, 1);
    setCentralWidget(central);

    connect(m_topBar, &TopBar::tabSelected, this, &MainWindow::showPage);
    connect(m_topBar, &TopBar::settingsClicked, this, [this]() {
        if (m_stack->currentIndex() == PageSettings)
            showPage(PageHome);
        else
            showPage(PageSettings);
    });
    connect(m_topBar, &TopBar::notificationsClicked, this, [this]() {
        m_topBar->setNotificationBadge(false);
        const QList<Device *> devices = m_ctx.devices->devices();
        if (devices.isEmpty())
            m_toasts->show(tr("No devices connected"), Toast::Info);
        else
            m_toasts->show(tr("%n device(s) connected", nullptr, devices.size()), Toast::Info);
    });
    connect(m_home, &HomePage::openEngineRequested, this, [this]() { showPage(PageEngine); });
    connect(m_home, &HomePage::openSettingsRequested, this, [this]() { showPage(PageSettings); });
    connect(m_home, &HomePage::openDeviceRequested, this, &MainWindow::openDevice);
    connect(m_engine, &EnginePage::openSettingsRequested, this, [this]() { showPage(PageSettings); });

    m_controller = new DeviceController(m_ctx, this);
    connect(m_controller, &DeviceController::permissionProblem, this, [this](Device *) { m_topBar->setNotificationBadge(true); });

    // Shortcuts
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_1), this, [this]() { showPage(PageHome); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_2), this, [this]() { showPage(PageEngine); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_3), this, [this]() { showPage(PageSonar); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_4), this, [this]() { showPage(PageMoments); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma), this, [this]() { showPage(PageSettings); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this, [this]() {
        m_quitting = true;
        qApp->quit();
    });

    setupTray();
    showPage(PageHome);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;
    m_tray = new QSystemTrayIcon(Icons::appIconSet(), this);
    m_tray->setToolTip(QStringLiteral("OpenGG"));
    auto *menu = new QMenu(this);
    QAction *open = menu->addAction(tr("Show OpenGG"));
    connect(open, &QAction::triggered, this, &MainWindow::bringToFront);
    QAction *engine = menu->addAction(tr("Engine"));
    connect(engine, &QAction::triggered, this, [this]() {
        bringToFront();
        showPage(PageEngine);
    });
    menu->addSeparator();
    QAction *quit = menu->addAction(tr("Quit"));
    connect(quit, &QAction::triggered, this, [this]() {
        m_quitting = true;
        qApp->quit();
    });
    m_tray->setContextMenu(menu);
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            if (isVisible() && !isMinimized())
                hide();
            else
                bringToFront();
        }
    });
    m_tray->show();
}

void MainWindow::showPage(int page)
{
    m_stack->setCurrentIndex(page);
    if (page == PageSettings) {
        m_topBar->setSettingsActive(true);
    } else {
        m_topBar->setSettingsActive(false);
        m_topBar->setCurrentTab(page);
    }
}

void MainWindow::openDevice(Device *device)
{
    showPage(PageEngine);
    m_engine->openDevice(device);
}

void MainWindow::bringToFront()
{
    showNormal();
    raise();
    activateWindow();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    m_toasts->relayout();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    const bool closeToTray = m_ctx.store->value(QStringLiteral("closeToTray"), true).toBool();
    if (!m_quitting && closeToTray && m_tray && m_tray->isVisible()) {
        hide();
        if (!m_trayHintShown) {
            m_trayHintShown = true;
            m_tray->showMessage(tr("Still running"), tr("OpenGG keeps your device settings alive from the tray. Use Quit in the tray menu to exit."),
                                Icons::appIconSet(), 4000);
        }
        event->ignore();
        return;
    }
    event->accept();
    qApp->quit();
}
