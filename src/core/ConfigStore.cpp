#include "core/ConfigStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>

Q_LOGGING_CATEGORY(lcConfig, "ssgg.config")

static ConfigStore *s_instance = nullptr;

ConfigStore::ConfigStore(const QString &filePath, QObject *parent)
    : QObject(parent)
{
    if (filePath.isEmpty()) {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        m_path = dir + QStringLiteral("/config.json");
    } else {
        m_path = filePath;
    }
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(400);
    connect(&m_saveTimer, &QTimer::timeout, this, &ConfigStore::save);
    if (!s_instance)
        s_instance = this;
}

ConfigStore::~ConfigStore()
{
    if (m_saveTimer.isActive()) {
        m_saveTimer.stop();
        save();
    }
    if (s_instance == this)
        s_instance = nullptr;
}

ConfigStore *ConfigStore::instance()
{
    return s_instance;
}

bool ConfigStore::load()
{
    QFile f(m_path);
    if (!f.exists()) {
        // One-time migration from the project's former name.
        const QString legacy = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
            + QStringLiteral("/steelseries-gg/config.json");
        if (QFile::exists(legacy)) {
            QDir().mkpath(QFileInfo(m_path).absolutePath());
            if (QFile::copy(legacy, m_path))
                qCInfo(lcConfig) << "Migrated settings from" << legacy;
        }
    }
    if (!f.exists()) {
        m_root = QJsonObject{{QStringLiteral("app"), QJsonObject()}, {QStringLiteral("devices"), QJsonObject()}};
        return true;
    }
    if (!f.open(QIODevice::ReadOnly)) {
        qCWarning(lcConfig) << "Cannot open" << m_path << f.errorString();
        return false;
    }
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qCWarning(lcConfig) << "Invalid config file, starting fresh:" << err.errorString();
        m_root = QJsonObject{{QStringLiteral("app"), QJsonObject()}, {QStringLiteral("devices"), QJsonObject()}};
        return false;
    }
    m_root = doc.object();
    if (!m_root.contains(QStringLiteral("app")))
        m_root.insert(QStringLiteral("app"), QJsonObject());
    if (!m_root.contains(QStringLiteral("devices")))
        m_root.insert(QStringLiteral("devices"), QJsonObject());
    return true;
}

bool ConfigStore::save()
{
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile f(m_path);
    if (!f.open(QIODevice::WriteOnly)) {
        qCWarning(lcConfig) << "Cannot write" << m_path << f.errorString();
        return false;
    }
    f.write(QJsonDocument(m_root).toJson(QJsonDocument::Indented));
    if (!f.commit()) {
        qCWarning(lcConfig) << "Commit failed" << f.errorString();
        return false;
    }
    return true;
}

void ConfigStore::scheduleSave()
{
    m_saveTimer.start();
}

// --- App settings -----------------------------------------------------------

QVariant ConfigStore::value(const QString &key, const QVariant &defaultValue) const
{
    const QJsonObject app = m_root.value(QStringLiteral("app")).toObject();
    if (!app.contains(key))
        return defaultValue;
    return app.value(key).toVariant();
}

void ConfigStore::setValue(const QString &key, const QVariant &value)
{
    QJsonObject app = m_root.value(QStringLiteral("app")).toObject();
    const QJsonValue jv = QJsonValue::fromVariant(value);
    if (app.value(key) == jv)
        return;
    app.insert(key, jv);
    m_root.insert(QStringLiteral("app"), app);
    scheduleSave();
    emit settingChanged(key, value);
}

// --- Device configs ---------------------------------------------------------

QJsonObject ConfigStore::deviceObject(const QString &deviceKey) const
{
    return m_root.value(QStringLiteral("devices")).toObject().value(deviceKey).toObject();
}

void ConfigStore::setDeviceObject(const QString &deviceKey, const QJsonObject &obj)
{
    QJsonObject devices = m_root.value(QStringLiteral("devices")).toObject();
    devices.insert(deviceKey, obj);
    m_root.insert(QStringLiteral("devices"), devices);
    scheduleSave();
}

QString ConfigStore::newId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

QList<ConfigStore::NamedConfig> ConfigStore::configs(const QString &deviceKey) const
{
    QList<NamedConfig> out;
    const QJsonArray arr = deviceObject(deviceKey).value(QStringLiteral("configs")).toArray();
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        out.append({o.value(QStringLiteral("id")).toString(), o.value(QStringLiteral("name")).toString(),
                    o.value(QStringLiteral("data")).toObject()});
    }
    return out;
}

ConfigStore::NamedConfig ConfigStore::config(const QString &deviceKey, const QString &id) const
{
    for (const NamedConfig &c : configs(deviceKey))
        if (c.id == id)
            return c;
    return {};
}

QString ConfigStore::activeConfigId(const QString &deviceKey) const
{
    return deviceObject(deviceKey).value(QStringLiteral("active")).toString();
}

ConfigStore::NamedConfig ConfigStore::activeConfig(const QString &deviceKey) const
{
    const QList<NamedConfig> list = configs(deviceKey);
    const QString id = activeConfigId(deviceKey);
    for (const NamedConfig &c : list)
        if (c.id == id)
            return c;
    return list.isEmpty() ? NamedConfig{} : list.first();
}

void ConfigStore::setActiveConfigId(const QString &deviceKey, const QString &id)
{
    QJsonObject dev = deviceObject(deviceKey);
    if (dev.value(QStringLiteral("active")).toString() == id)
        return;
    dev.insert(QStringLiteral("active"), id);
    setDeviceObject(deviceKey, dev);
    emit activeConfigChanged(deviceKey, id);
}

void ConfigStore::ensureDefaultConfig(const QString &deviceKey, const QJsonObject &defaultData, const QString &defaultName)
{
    if (!configs(deviceKey).isEmpty()) {
        if (activeConfigId(deviceKey).isEmpty())
            setActiveConfigId(deviceKey, configs(deviceKey).first().id);
        return;
    }
    const NamedConfig c = addConfig(deviceKey, defaultName, defaultData);
    setActiveConfigId(deviceKey, c.id);
}

ConfigStore::NamedConfig ConfigStore::addConfig(const QString &deviceKey, const QString &name, const QJsonObject &data)
{
    QJsonObject dev = deviceObject(deviceKey);
    QJsonArray arr = dev.value(QStringLiteral("configs")).toArray();
    NamedConfig c{newId(), name, data};
    arr.append(QJsonObject{{QStringLiteral("id"), c.id}, {QStringLiteral("name"), c.name}, {QStringLiteral("data"), c.data}});
    dev.insert(QStringLiteral("configs"), arr);
    setDeviceObject(deviceKey, dev);
    emit configsChanged(deviceKey);
    return c;
}

void ConfigStore::updateConfigData(const QString &deviceKey, const QString &id, const QJsonObject &data)
{
    QJsonObject dev = deviceObject(deviceKey);
    QJsonArray arr = dev.value(QStringLiteral("configs")).toArray();
    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject o = arr.at(i).toObject();
        if (o.value(QStringLiteral("id")).toString() == id) {
            if (o.value(QStringLiteral("data")).toObject() == data)
                return;
            o.insert(QStringLiteral("data"), data);
            arr.replace(i, o);
            dev.insert(QStringLiteral("configs"), arr);
            setDeviceObject(deviceKey, dev);
            emit configDataChanged(deviceKey, id);
            return;
        }
    }
}

void ConfigStore::renameConfig(const QString &deviceKey, const QString &id, const QString &name)
{
    QJsonObject dev = deviceObject(deviceKey);
    QJsonArray arr = dev.value(QStringLiteral("configs")).toArray();
    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject o = arr.at(i).toObject();
        if (o.value(QStringLiteral("id")).toString() == id) {
            o.insert(QStringLiteral("name"), name);
            arr.replace(i, o);
            dev.insert(QStringLiteral("configs"), arr);
            setDeviceObject(deviceKey, dev);
            emit configsChanged(deviceKey);
            return;
        }
    }
}

bool ConfigStore::removeConfig(const QString &deviceKey, const QString &id)
{
    QJsonObject dev = deviceObject(deviceKey);
    QJsonArray arr = dev.value(QStringLiteral("configs")).toArray();
    if (arr.size() <= 1)
        return false; // keep at least one config
    for (int i = 0; i < arr.size(); ++i) {
        if (arr.at(i).toObject().value(QStringLiteral("id")).toString() == id) {
            arr.removeAt(i);
            dev.insert(QStringLiteral("configs"), arr);
            const bool wasActive = dev.value(QStringLiteral("active")).toString() == id;
            if (wasActive)
                dev.insert(QStringLiteral("active"), arr.first().toObject().value(QStringLiteral("id")).toString());
            setDeviceObject(deviceKey, dev);
            emit configsChanged(deviceKey);
            if (wasActive)
                emit activeConfigChanged(deviceKey, dev.value(QStringLiteral("active")).toString());
            return true;
        }
    }
    return false;
}
