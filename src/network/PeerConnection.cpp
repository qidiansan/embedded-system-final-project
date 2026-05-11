#include "PeerConnection.h"
#include "protocol/MessageSerializer.h"

PeerConnection::PeerConnection(QTcpSocket* socket, QObject* parent)
    : QObject(parent)
    , m_socket(socket)
{
    m_socket->setParent(this);
    connect(m_socket, &QTcpSocket::readyRead, this, &PeerConnection::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &PeerConnection::onDisconnected);
}

PeerConnection::~PeerConnection() {
    if (m_socket->state() == QAbstractSocket::ConnectedState)
        m_socket->disconnectFromHost();
}

bool PeerConnection::isConnected() const {
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

QString PeerConnection::peerAddress() const {
    return m_socket->peerAddress().toString() + ":" + QString::number(m_socket->peerPort());
}

// -------------------- Send Methods --------------------
void PeerConnection::sendFrame(MessageType type, const QByteArray& payload) {
    if (!isConnected()) return;
    QByteArray frame = MessageFramer::frameMessage(type, payload);
    m_socket->write(frame);
}

void PeerConnection::sendHandshake(const QString& hostname) {
    sendFrame(MessageType::MSG_HANDSHAKE_REQ,
              MessageSerializer::serializeHandshakeReq(hostname));
}

void PeerConnection::sendFileInfo(const FileInfo& info) {
    sendFrame(MessageType::MSG_FILE_INFO,
              MessageSerializer::serializeFileInfo(info));
}

void PeerConnection::sendFileChunk(quint32 index, const QByteArray& data) {
    sendFrame(MessageType::MSG_FILE_CHUNK,
              MessageSerializer::serializeFileChunk(index, data));
}

void PeerConnection::sendFileComplete(const QString& relativePath, const QString& fileMD5) {
    sendFrame(MessageType::MSG_FILE_COMPLETE,
              MessageSerializer::serializeFileComplete(relativePath, fileMD5));
}

void PeerConnection::sendDirCreate(const QString& relativeDirPath) {
    sendFrame(MessageType::MSG_DIR_CREATE,
              MessageSerializer::serializeDirCreate(relativeDirPath));
}

void PeerConnection::sendTransferComplete() {
    sendFrame(MessageType::MSG_TRANSFER_COMPLETE,
              MessageSerializer::serializeTransferComplete());
}

void PeerConnection::sendError(quint32 code, const QString& desc) {
    sendFrame(MessageType::MSG_ERROR,
              MessageSerializer::serializeError(code, desc));
}

void PeerConnection::sendCancel() {
    sendFrame(MessageType::MSG_CANCEL,
              MessageSerializer::serializeCancel());
}

void PeerConnection::sendFileRequest(const QString& remotePath) {
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << remotePath;
    sendFrame(MessageType::MSG_FILE_REQUEST, payload);
}

// -------------------- Receive --------------------
void PeerConnection::onReadyRead() {
    QByteArray data = m_socket->readAll();
    m_framer.feedData(data);

    MessageType type;
    QByteArray payload;
    while (true) {
        FrameResult result = m_framer.tryParseFrame(type, payload);
        if (result == FrameResult::Complete) {
            dispatchMessage(type, payload);
        } else if (result == FrameResult::Incomplete) {
            break;
        } else { // Invalid
            emit logMessage("Warning: Invalid/corrupt frame discarded");
        }
    }
}

void PeerConnection::onDisconnected() {
    emit logMessage(QString("Disconnected from %1").arg(peerAddress()));
    emit disconnected();
}

void PeerConnection::dispatchMessage(MessageType type, const QByteArray& payload) {
    switch (type) {
    case MessageType::MSG_HANDSHAKE_REQ: {
        QString hostname;
        if (MessageSerializer::deserializeHandshakeReq(payload, hostname)) {
            // Auto-reply with ACK
            sendFrame(MessageType::MSG_HANDSHAKE_ACK,
                      MessageSerializer::serializeHandshakeAck(
                          QTcpSocket::tr("Peer")));
            emit handshakeReceived(PROTOCOL_VERSION, hostname);
        }
        break;
    }
    case MessageType::MSG_HANDSHAKE_ACK: {
        QString hostname;
        if (MessageSerializer::deserializeHandshakeAck(payload, hostname)) {
            emit handshakeReceived(PROTOCOL_VERSION, hostname);
        }
        break;
    }
    case MessageType::MSG_FILE_INFO: {
        FileInfo info;
        if (MessageSerializer::deserializeFileInfo(payload, info))
            emit fileInfoReceived(info);
        break;
    }
    case MessageType::MSG_FILE_CHUNK: {
        quint32 index;
        QByteArray data;
        if (MessageSerializer::deserializeFileChunk(payload, index, data))
            emit chunkReceived(index, data);
        break;
    }
    case MessageType::MSG_FILE_COMPLETE: {
        QString path, md5;
        if (MessageSerializer::deserializeFileComplete(payload, path, md5))
            emit fileCompleteReceived(path, md5);
        break;
    }
    case MessageType::MSG_DIR_CREATE: {
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::BigEndian);
        QString dirPath;
        stream >> dirPath;
        if (stream.status() == QDataStream::Ok)
            emit dirCreateReceived(dirPath);
        break;
    }
    case MessageType::MSG_TRANSFER_COMPLETE:
        emit transferCompleteReceived();
        break;
    case MessageType::MSG_ERROR: {
        quint32 code;
        QString desc;
        if (MessageSerializer::deserializeError(payload, code, desc))
            emit errorReceived(code, desc);
        break;
    }
    case MessageType::MSG_CANCEL:
        emit cancelReceived();
        break;
    case MessageType::MSG_FILE_REQUEST: {
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::BigEndian);
        QString remotePath;
        stream >> remotePath;
        if (stream.status() == QDataStream::Ok)
            emit fileRequestReceived(remotePath);
        break;
    }
    }
}
