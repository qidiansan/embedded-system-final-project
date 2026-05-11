#ifndef PEERCONNECTION_H
#define PEERCONNECTION_H

#include "protocol/ProtocolDefs.h"
#include "protocol/MessageFramer.h"
#include <QTcpSocket>
#include <QQueue>

class PeerConnection : public QObject {
    Q_OBJECT
public:
    explicit PeerConnection(QTcpSocket* socket, QObject* parent = nullptr);
    ~PeerConnection();

    bool isConnected() const;
    QString peerAddress() const;

    // --- Send methods ---
    void sendHandshake(const QString& hostname);
    void sendFileInfo(const FileInfo& info);
    void sendFileChunk(quint32 index, const QByteArray& data);
    void sendFileComplete(const QString& relativePath, const QString& fileMD5);
    void sendDirCreate(const QString& relativeDirPath);
    void sendTransferComplete();
    void sendError(quint32 code, const QString& desc);
    void sendCancel();
    void sendFileRequest(const QString& remotePath);

signals:
    void handshakeReceived(const QString& version, const QString& hostname);
    void fileInfoReceived(const FileInfo& info);
    void chunkReceived(quint32 index, const QByteArray& data);
    void fileCompleteReceived(const QString& relativePath, const QString& fileMD5);
    void dirCreateReceived(const QString& relativeDirPath);
    void transferCompleteReceived();
    void errorReceived(quint32 code, const QString& desc);
    void cancelReceived();
    void fileRequestReceived(const QString& remotePath);
    void disconnected();
    void logMessage(const QString& msg);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    void sendFrame(MessageType type, const QByteArray& payload);
    void dispatchMessage(MessageType type, const QByteArray& payload);

    QTcpSocket*   m_socket;
    MessageFramer m_framer;
};

#endif // PEERCONNECTION_H
