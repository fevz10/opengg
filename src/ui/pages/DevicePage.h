#pragma once

#include "ui/AppContext.h"

#include <QJsonObject>
#include <QList>
#include <QWidget>

class Device;
class ConfigList;
class NavTab;
class QButtonGroup;
class QLabel;
class QStackedWidget;

/// Base class of the per-device settings tabs ("Illumination", "Buttons", ...).
class ConfigTab : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigTab(QWidget *parent = nullptr)
        : QWidget(parent)
    {
    }
    /// Loads the given config into the widgets without emitting configEdited().
    virtual void loadConfig(const QJsonObject &data) = 0;

signals:
    /// The user changed something; `data` is the complete new config.
    void configEdited(const QJsonObject &data);
};

/// Device detail view: preview + configs on the left, tabs on the right.
class DevicePage : public QWidget
{
    Q_OBJECT
public:
    DevicePage(const AppContext &ctx, Device *device, QWidget *parent = nullptr);

    Device *device() const { return m_device; }
    int tabCount() const { return m_tabs.size(); }
    void setCurrentTab(int index);

signals:
    void backRequested();

private:
    void addTab(const QString &title, ConfigTab *tab);
    void reloadConfigs();
    void loadActiveIntoTabs();
    void onConfigEdited(const QJsonObject &data);
    void refreshStatus();

    AppContext m_ctx;
    Device *m_device;
    QLabel *m_statusLabel;
    QLabel *m_batteryLabel;
    ConfigList *m_configList;
    QButtonGroup *m_tabGroup;
    QList<NavTab *> m_tabButtons;
    QStackedWidget *m_tabStack;
    QList<ConfigTab *> m_tabs;
    class QHBoxLayout *m_tabBar;
    bool m_loading = false;
};
