#pragma once

#include "ui/AppContext.h"

#include <QWidget>

class Device;
class QLabel;
class QVBoxLayout;

/// Landing page: greeting, quick gear status and tips.
class HomePage : public QWidget
{
    Q_OBJECT
public:
    explicit HomePage(const AppContext &ctx, QWidget *parent = nullptr);

signals:
    void openEngineRequested();
    void openDeviceRequested(Device *device);
    void openSettingsRequested();

private:
    void rebuildGear();
    QWidget *makeGearRow(Device *device);
    QWidget *makeTipCard(const QString &icon, const QString &title, const QString &text);

    AppContext m_ctx;
    QLabel *m_greeting;
    QVBoxLayout *m_gearLayout;
    QList<QWidget *> m_gearRows;
};
