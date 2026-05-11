#ifndef PEERSERVER_H
#define PEERSERVER_H

#include "protocol/ProtocolDefs.h"
#include <QTcpServer>
#include <QTcpSocket>

class PeerServer : public QObject {
    Q_OBJECT
public:
    explicit PeerServer(QObject* parent = nullptr);

    bool startListening(quint16 port = DEFAULT_PORT);
    void stop();

    bool isListening() const;
    quint16 serverPort() const;

signals:
    void clientConnected(QTcpSocket* socket);
    void serverError(const QString& error);
    void logMessage(const QString& msg);

private slots:
    void onNewConnection();

private:
    QTcpServer* m_server;
};

#endif // PEERSERVER_H
