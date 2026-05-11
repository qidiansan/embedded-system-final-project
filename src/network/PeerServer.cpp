#include "PeerServer.h"
#include <QHostAddress>

PeerServer::PeerServer(QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &PeerServer::onNewConnection);
}

bool PeerServer::startListening(quint16 port) {
    if (m_server->isListening()) {
        m_server->close();
    }
    if (!m_server->listen(QHostAddress::Any, port)) {
        emit serverError(m_server->errorString());
        return false;
    }
    emit logMessage(QString("Server listening on port %1").arg(port));
    return true;
}

void PeerServer::stop() {
    if (m_server->isListening()) {
        m_server->close();
        emit logMessage("Server stopped.");
    }
}

bool PeerServer::isListening() const {
    return m_server->isListening();
}

quint16 PeerServer::serverPort() const {
    return m_server->serverPort();
}

void PeerServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        emit logMessage(QString("New connection from %1:%2")
            .arg(socket->peerAddress().toString())
            .arg(socket->peerPort()));
        emit clientConnected(socket);
    }
}
