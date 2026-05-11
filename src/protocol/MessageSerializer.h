#ifndef MESSAGESERIALIZER_H
#define MESSAGESERIALIZER_H

#include "ProtocolDefs.h"
#include <QByteArray>
#include <QString>
#include <QVector>

class MessageSerializer {
public:
    // --- Handshake ---
    static QByteArray serializeHandshakeReq(const QString& hostname);
    static bool deserializeHandshakeReq(const QByteArray& payload, QString& hostname);
    static QByteArray serializeHandshakeAck(const QString& hostname);
    static bool deserializeHandshakeAck(const QByteArray& payload, QString& hostname);

    // --- File transfer ---
    static QByteArray serializeFileInfo(const FileInfo& info);
    static bool deserializeFileInfo(const QByteArray& payload, FileInfo& info);

    static QByteArray serializeFileChunk(quint32 index, const QByteArray& data);
    static bool deserializeFileChunk(const QByteArray& payload, quint32& index, QByteArray& data);

    static QByteArray serializeFileComplete(const QString& relativePath, const QString& fileMD5);
    static bool deserializeFileComplete(const QByteArray& payload, QString& relativePath, QString& fileMD5);

    // --- Directory ---
    static QByteArray serializeDirCreate(const QString& relativeDirPath);

    // --- Control ---
    static QByteArray serializeTransferComplete();
    static QByteArray serializeError(quint32 code, const QString& desc);
    static bool deserializeError(const QByteArray& payload, quint32& code, QString& desc);
    static QByteArray serializeCancel();
};

#endif // MESSAGESERIALIZER_H
