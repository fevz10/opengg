#pragma once

#include "core/ConfigStore.h"

#include <QList>
#include <QWidget>

class QVBoxLayout;

/// "CONFIGS" panel of the device page: list of named configs for a device.
class ConfigList : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigList(QWidget *parent = nullptr);

    void setConfigs(const QList<ConfigStore::NamedConfig> &configs, const QString &activeId);

signals:
    void configSelected(const QString &id);
    void addRequested();
    void renameRequested(const QString &id);
    void duplicateRequested(const QString &id);
    void deleteRequested(const QString &id);

private:
    QVBoxLayout *m_rows;
    QList<QWidget *> m_rowWidgets;
};
