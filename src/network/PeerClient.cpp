#include "PeerClient.h"
#include <QNetworkProxy>

PeerClient::PeerClient(QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_port(DEFAULT_PORT)
{
    m_socket->setProxy(QNetworkProxy::NoProxy);
    connect(m_socket, &QTcpSocket::connected, this, &PeerClient::onConnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &PeerClient::onErrorOccurred);
}

void PeerClient::connectToServer(const QString& host, quint16 port) {
    m_host = host;
    m_port = port;
    emit logMessage(QString("Connecting to %1:%2...").arg(host).arg(port));
    m_socket->connectToHost(host, port);
}

void PeerClient::disconnect() {
    m_socket->disconnectFromHost();
}

bool PeerClient::isConnected() const {
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void PeerClient::onConnected() {
    emit logMessage(QString("Connected to %1:%2").arg(m_host).arg(m_port));
    emit connected(m_socket);
}

void PeerClient::onErrorOccurred(QAbstractSocket::SocketError error) {
    Q_UNUSED(error)
    emit connectionError(m_socket->errorString());
}
