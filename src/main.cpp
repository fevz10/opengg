#include "core/Autostart.h"
#include "core/ConfigStore.h"
#include "core/Icons.h"
#include "core/SingleInstance.h"
#include "core/Theme.h"
#include "devices/Device.h"
#include "devices/DeviceManager.h"
#include "hid/HidTransport.h"
#include "ui/AppContext.h"
#include "ui/MainWindow.h"
#include "ui/pages/DevicePage.h"
#include "ui/pages/EnginePage.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QLoggingCategory>
#include <QSystemTrayIcon>
#include <QThread>
#include <QTimer>
#include <QTranslator>

#include <unistd.h>

namespace {

void installTranslations(QApplication &app, const QString &requested)
{
    QLocale locale = QLocale::system();
    if (requested != QLatin1String("system") && !requested.isEmpty())
        locale = QLocale(requested);
    if (locale.language() == QLocale::English || locale.language() == QLocale::C)
        return;

    auto *qtTr = new QTranslator(&app);
    if (qtTr->load(locale, QStringLiteral("qtbase"), QStringLiteral("_"), QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        app.installTranslator(qtTr);

    auto *appTr = new QTranslator(&app);
    if (appTr->load(locale, QStringLiteral("opengg"), QStringLiteral("_"), QStringLiteral(":/i18n")))
        app.installTranslator(appTr);
}

/// Renders every page to PNG files (used for visual checks in CI/dev).
void runScreenshotMode(MainWindow *window, const AppContext &ctx, const QString &dir)
{
    QDir().mkpath(dir);
    auto grab = [window, dir](const QString &name) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        window->grab().save(dir + QStringLiteral("/%1.png").arg(name));
    };
    window->showPage(MainWindow::PageHome);
    grab(QStringLiteral("01-home"));
    window->showPage(MainWindow::PageEngine);
    grab(QStringLiteral("02-engine"));

    int n = 3;
    for (Device *d : ctx.devices->devices()) {
        window->openDevice(d);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        // walk the tabs
        DevicePage *page = window->enginePage()->devicePage();
        const int tabCount = page ? page->tabCount() : 1;
        for (int t = 0; t < tabCount; ++t) {
            if (page)
                page->setCurrentTab(t);
            grab(QStringLiteral("%1-%2-tab%3").arg(n, 2, 10, QLatin1Char('0')).arg(d->key()).arg(t + 1));
        }
        ++n;
        window->enginePage()->showGrid();
    }
    window->showPage(MainWindow::PageSonar);
    grab(QStringLiteral("%1-sonar").arg(n++, 2, 10, QLatin1Char('0')));
    window->showPage(MainWindow::PageMoments);
    grab(QStringLiteral("%1-moments").arg(n++, 2, 10, QLatin1Char('0')));
    window->showPage(MainWindow::PageSettings);
    grab(QStringLiteral("%1-settings").arg(n++, 2, 10, QLatin1Char('0')));
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("opengg"));
    QCoreApplication::setOrganizationName(QStringLiteral("opengg"));
    QCoreApplication::setApplicationVersion(QStringLiteral(SSGG_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral(SSGG_APP_ID));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("OpenGG"));
    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("OpenGG - unofficial SteelSeries GG alternative for Linux"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption demoOpt(QStringLiteral("demo"), QStringLiteral("Simulate the supported devices (no hardware access)."));
    QCommandLineOption minimizedOpt(QStringLiteral("minimized"), QStringLiteral("Start hidden in the system tray."));
    QCommandLineOption langOpt(QStringLiteral("lang"), QStringLiteral("Override the UI language (en, tr)."), QStringLiteral("code"));
    QCommandLineOption configOpt(QStringLiteral("config"), QStringLiteral("Use an alternative config file."), QStringLiteral("path"));
    QCommandLineOption screenshotOpt(QStringLiteral("screenshot"), QStringLiteral("Render every page to PNG files in <dir> and exit."),
                                     QStringLiteral("dir"));
    QCommandLineOption verboseOpt(QStringLiteral("verbose"), QStringLiteral("Verbose HID logging on stderr."));
    parser.addOption(demoOpt);
    parser.addOption(minimizedOpt);
    parser.addOption(langOpt);
    parser.addOption(configOpt);
    parser.addOption(screenshotOpt);
    parser.addOption(verboseOpt);
    parser.process(app);

    const bool demo = parser.isSet(demoOpt);
    const bool screenshot = parser.isSet(screenshotOpt);
    if (parser.isSet(verboseOpt))
        QLoggingCategory::setFilterRules(QStringLiteral("ssgg.*=true"));
    else
        QLoggingCategory::setFilterRules(QStringLiteral("ssgg.*.debug=false"));

    ConfigStore store(parser.value(configOpt));
    store.load();
    Autostart::migrateLegacyEntry(store.value(QStringLiteral("startMinimized"), true).toBool());

    Theme::apply(app);
    app.setWindowIcon(Icons::appIconSet());
    installTranslations(app, parser.isSet(langOpt) ? parser.value(langOpt) : store.value(QStringLiteral("language"), QStringLiteral("system")).toString());

    // Single instance (per user): forward to the running one.
    SingleInstance instance(QStringLiteral("opengg-%1").arg(getuid()));
    if (!screenshot && !demo && !instance.tryStart()) {
        instance.sendToPrimary(QByteArrayLiteral("show"));
        return 0;
    }

    // HID worker thread
    QThread hidThread;
    hidThread.setObjectName(QStringLiteral("hid-io"));
    auto *transport = new HidTransport(demo);
    transport->moveToThread(&hidThread);
    QObject::connect(&hidThread, &QThread::finished, transport, &QObject::deleteLater);
    hidThread.start();

    int rc = 0;
    {
        // Scoped so that devices and windows are destroyed while the HID
        // thread is still running (their destructors close the handles).
        DeviceManager devices(transport);

        AppContext ctx;
        ctx.devices = &devices;
        ctx.store = &store;
        ctx.demo = demo;

        MainWindow window(ctx, transport);
        QObject::connect(&instance, &SingleInstance::messageReceived, &window, [&window](const QByteArray &) { window.bringToFront(); });

        devices.start();

        if (screenshot) {
            window.show();
            QTimer::singleShot(400, &app, [&]() {
                runScreenshotMode(&window, ctx, parser.value(screenshotOpt));
                app.quit();
            });
        } else if (parser.isSet(minimizedOpt) && QSystemTrayIcon::isSystemTrayAvailable()) {
            // stay in the tray
        } else {
            window.show();
        }

        rc = app.exec();
        devices.stop();
    }

    hidThread.quit();
    hidThread.wait(2000);
    store.save();
    return rc;
}
