#pragma once

#include "ui/AppContext.h"

#include <QMainWindow>

class TopBar;
class HomePage;
class EnginePage;
class SettingsPage;
class ToastHost;
class DeviceController;
class HidTransport;
class Device;
class QStackedWidget;
class QSystemTrayIcon;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    enum Page { PageHome = 0, PageEngine, PageSonar, PageMoments, PageSettings };

    MainWindow(const AppContext &ctx, HidTransport *transport, QWidget *parent = nullptr);
    ~MainWindow() override;

    void showPage(int page);
    void openDevice(Device *device);
    EnginePage *enginePage() const { return m_engine; }
    ToastHost *toasts() const { return m_toasts; }

    /// Brings the window to the front (used by the tray and second instances).
    void bringToFront();

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupTray();

    AppContext m_ctx;
    TopBar *m_topBar;
    QStackedWidget *m_stack;
    HomePage *m_home;
    EnginePage *m_engine;
    SettingsPage *m_settings;
    ToastHost *m_toasts;
    DeviceController *m_controller;
    QSystemTrayIcon *m_tray = nullptr;
    bool m_trayHintShown = false;
    bool m_quitting = false;
};
