#pragma once

#include <QObject>
#include <QString>

class QLocalServer;

/**
 * Makes sure only one instance of the application runs per user. A second
 * instance forwards its request (e.g. "show") to the running one and exits.
 */
class SingleInstance : public QObject
{
    Q_OBJECT
public:
    explicit SingleInstance(const QString &key, QObject *parent = nullptr);
    ~SingleInstance() override;

    /// Returns true if this process is the primary instance.
    bool tryStart();
    /// Sends a message to the primary instance (used by secondary instances).
    bool sendToPrimary(const QByteArray &message, int timeoutMs = 1000);

signals:
    void messageReceived(const QByteArray &message);

private:
    QString m_key;
    QLocalServer *m_server = nullptr;
};
