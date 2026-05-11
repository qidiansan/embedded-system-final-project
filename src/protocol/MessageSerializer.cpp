#include "MessageSerializer.h"
#include <QDataStream>
#include <QBuffer>

// ---------------- helpers ------------------
static QByteArray pack(std::function<void(QDataStream&)> writer) {
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    writer(stream);
    return payload;
}

// ---------------- Handshake ----------------
QByteArray MessageSerializer::serializeHandshakeReq(const QString& hostname) {
    return pack([&](QDataStream& s) { s << PROTOCOL_VERSION << hostname; });
}

bool MessageSerializer::deserializeHandshakeReq(const QByteArray& payload, QString& hostname) {
    QDataStream stream(payload);
    stream.setByteOrder(QDataStream::BigEndian);
    QString version;
    stream >> version >> hostname;
    return stream.status() == QDataStream::Ok;
}

QByteArray MessageSerializer::serializeHandshakeAck(const QString& hostname) {
    return pack([&](QDataStream& s) { s << PROTOCOL_VERSION << hostname; });
}

bool MessageSerializer::deserializeHandshakeAck(const QByteArray& payload, QString& hostname) {
    QDataStream stream(payload);
    stream.setByteOrder(QDataStream::BigEndian);
    QString version;
    stream >> version >> hostname;
    return stream.status() == QDataStream::Ok;
}

// ---------------- File Info ----------------
QByteArray MessageSerializer::serializeFileInfo(const FileInfo& info) {
    return pack([&](QDataStream& s) {
        s << info.relativePath << info.fileSize << info.fileMD5
          << info.chunkSize << info.totalChunks;
    });
}

bool MessageSerializer::deserializeFileInfo(const QByteArray& payload, FileInfo& info) {
    QDataStream stream(payload);
    stream.setByteOrder(QDataStream::BigEndian);
    stream >> info.relativePath >> info.fileSize >> info.fileMD5
           >> info.chunkSize >> info.totalChunks;
    return stream.status() == QDataStream::Ok;
}

// ---------------- File Chunk ----------------
QByteArray MessageSerializer::serializeFileChunk(quint32 index, const QByteArray& data) {
    return pack([&](QDataStream& s) { s << index << data; });
}

bool MessageSerializer::deserializeFileChunk(const QByteArray& payload, quint32& index, QByteArray& data) {
    QDataStream stream(payload);
    stream.setByteOrder(QDataStream::BigEndian);
    stream >> index >> data;
    return stream.status() == QDataStream::Ok;
}

// ---------------- File Complete ----------------
QByteArray MessageSerializer::serializeFileComplete(const QString& relativePath, const QString& fileMD5) {
    return pack([&](QDataStream& s) { s << relativePath << fileMD5; });
}

bool MessageSerializer::deserializeFileComplete(const QByteArray& payload, QString& relativePath, QString& fileMD5) {
    QDataStream stream(payload);
    stream.setByteOrder(QDataStream::BigEndian);
    stream >> relativePath >> fileMD5;
    return stream.status() == QDataStream::Ok;
}

// ---------------- Dir Create ----------------
QByteArray MessageSerializer::serializeDirCreate(const QString& relativeDirPath) {
    return pack([&](QDataStream& s) { s << relativeDirPath; });
}

// ---------------- Control ----------------
QByteArray MessageSerializer::serializeTransferComplete() {
    return pack([](QDataStream&) { /* empty */ });
}

QByteArray MessageSerializer::serializeError(quint32 code, const QString& desc) {
    return pack([&](QDataStream& s) { s << code << desc; });
}

bool MessageSerializer::deserializeError(const QByteArray& payload, quint32& code, QString& desc) {
    QDataStream stream(payload);
    stream.setByteOrder(QDataStream::BigEndian);
    stream >> code >> desc;
    return stream.status() == QDataStream::Ok;
}

QByteArray MessageSerializer::serializeCancel() {
    return pack([](QDataStream&) { /* empty */ });
}
