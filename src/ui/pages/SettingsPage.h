#pragma once

#include "ui/AppContext.h"

#include <QWidget>

class ToggleSwitch;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class HidTransport;

/// Application settings (gear icon): general, permissions, diagnostics, about.
class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    SettingsPage(const AppContext &ctx, HidTransport *transport, QWidget *parent = nullptr);

    void appendLog(const QString &line);

private:
    void refreshPermissions();
    void installUdevRules();

    AppContext m_ctx;
    QComboBox *m_language;
    ToggleSwitch *m_autostart;
    ToggleSwitch *m_startMinimized;
    ToggleSwitch *m_closeToTray;
    ToggleSwitch *m_notifications;
    QLabel *m_permissionStatus;
    QLabel *m_permissionIcon;
    QPushButton *m_installRules;
    QPlainTextEdit *m_log;
    QLabel *m_restartHint;
};
