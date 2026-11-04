#include "core/SingleInstance.h"

#include <QLocalServer>
#include <QLocalSocket>

SingleInstance::SingleInstance(const QString &key, QObject *parent)
    : QObject(parent)
    , m_key(key)
{
}

SingleInstance::~SingleInstance()
{
    if (m_server) {
        m_server->close();
        QLocalServer::removeServer(m_key);
    }
}

bool SingleInstance::tryStart()
{
    // Probe for a live primary instance first.
    {
        QLocalSocket probe;
        probe.connectToServer(m_key);
        if (probe.waitForConnected(300)) {
            probe.disconnectFromServer();
            return false;
        }
    }
    // Stale socket file from a crashed instance: remove and listen.
    QLocalServer::removeServer(m_key);
    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    if (!m_server->listen(m_key)) {
        delete m_server;
        m_server = nullptr;
        return true; // cannot listen, but behave as primary anyway
    }
    connect(m_server, &QLocalServer::newConnection, this, [this]() {
        while (QLocalSocket *sock = m_server->nextPendingConnection()) {
            connect(sock, &QLocalSocket::readyRead, this, [this, sock]() {
                const QByteArray msg = sock->readAll();
                if (!msg.isEmpty())
                    emit messageReceived(msg);
                sock->disconnectFromServer();
            });
            connect(sock, &QLocalSocket::disconnected, sock, &QObject::deleteLater);
        }
    });
    return true;
}

bool SingleInstance::sendToPrimary(const QByteArray &message, int timeoutMs)
{
    QLocalSocket sock;
    sock.connectToServer(m_key);
    if (!sock.waitForConnected(timeoutMs))
        return false;
    sock.write(message);
    sock.flush();
    sock.waitForBytesWritten(timeoutMs);
    sock.disconnectFromServer();
    return true;
}
