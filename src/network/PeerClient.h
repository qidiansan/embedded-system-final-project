#ifndef PEERCLIENT_H
#define PEERCLIENT_H

#include "protocol/ProtocolDefs.h"
#include <QTcpSocket>

class PeerClient : public QObject {
    Q_OBJECT
public:
    explicit PeerClient(QObject* parent = nullptr);

    void connectToServer(const QString& host, quint16 port = DEFAULT_PORT);
    void disconnect();

    bool isConnected() const;

signals:
    void connected(QTcpSocket* socket);
    void connectionError(const QString& error);
    void logMessage(const QString& msg);

private slots:
    void onConnected();
    void onErrorOccurred(QAbstractSocket::SocketError error);

private:
    QTcpSocket* m_socket;
    QString     m_host;
    quint16     m_port;
};

#endif // PEERCLIENT_H
