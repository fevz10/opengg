#pragma once

#include "ui/AppContext.h"

#include <QWidget>

class Device;
class FlowLayout;
class QStackedWidget;
class QLabel;
class EmptyState;

/// "Engine" tab: the gear grid. Device pages are pushed on top of it.
class EnginePage : public QWidget
{
    Q_OBJECT
public:
    explicit EnginePage(const AppContext &ctx, QWidget *parent = nullptr);

    void openDevice(Device *device);
    void showGrid();
    bool isShowingDevice() const;
    /// The device page currently shown (nullptr when the grid is visible).
    class DevicePage *devicePage() const;

signals:
    void openSettingsRequested();

private:
    void rebuildGrid();
    QWidget *makeAppCard(const QString &icon, const QString &name, const QString &description);
    QWidget *makeUnsupportedCard(const QString &product, quint16 pid);

    AppContext m_ctx;
    QStackedWidget *m_stack;
    QWidget *m_gridPage;
    FlowLayout *m_gearFlow;
    QWidget *m_gearContainer;
    EmptyState *m_empty;
    QWidget *m_unsupportedBox;
    QWidget *m_devicePage = nullptr;
    QList<QWidget *> m_cards;
};
