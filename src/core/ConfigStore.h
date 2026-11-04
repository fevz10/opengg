#pragma once

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariant>

/**
 * JSON backed persistence for application settings and the per device
 * "configs" (what SteelSeries GG calls the profiles of a device).
 *
 * File: $XDG_CONFIG_HOME/opengg/config.json (settings from the former
 * "steelseries-gg" location are migrated automatically).
 */
class ConfigStore : public QObject
{
    Q_OBJECT
public:
    struct NamedConfig {
        QString id;
        QString name;
        QJsonObject data;
    };

    explicit ConfigStore(const QString &filePath = QString(), QObject *parent = nullptr);
    ~ConfigStore() override;

    static ConfigStore *instance();

    QString filePath() const { return m_path; }
    bool load();
    bool save();
    void scheduleSave();

    // --- Application settings -------------------------------------------
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);

    // --- Device configs ---------------------------------------------------
    QList<NamedConfig> configs(const QString &deviceKey) const;
    NamedConfig config(const QString &deviceKey, const QString &id) const;
    QString activeConfigId(const QString &deviceKey) const;
    NamedConfig activeConfig(const QString &deviceKey) const;
    void setActiveConfigId(const QString &deviceKey, const QString &id);

    /// Creates a default config when the device has none yet.
    void ensureDefaultConfig(const QString &deviceKey, const QJsonObject &defaultData, const QString &defaultName);
    NamedConfig addConfig(const QString &deviceKey, const QString &name, const QJsonObject &data);
    void updateConfigData(const QString &deviceKey, const QString &id, const QJsonObject &data);
    void renameConfig(const QString &deviceKey, const QString &id, const QString &name);
    bool removeConfig(const QString &deviceKey, const QString &id);

signals:
    void settingChanged(const QString &key, const QVariant &value);
    void configsChanged(const QString &deviceKey);
    void activeConfigChanged(const QString &deviceKey, const QString &id);
    void configDataChanged(const QString &deviceKey, const QString &id);

private:
    QJsonObject deviceObject(const QString &deviceKey) const;
    void setDeviceObject(const QString &deviceKey, const QJsonObject &obj);
    static QString newId();

    QString m_path;
    QJsonObject m_root;
    QTimer m_saveTimer;
};
