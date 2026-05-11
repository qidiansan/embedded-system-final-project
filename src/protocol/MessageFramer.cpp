#include "MessageFramer.h"
#include "utils/Checksum.h"
#include <QDataStream>
#include <QBuffer>

static constexpr int HEADER_SIZE = 12;  // magic(4) + length(4) + type(4)
static constexpr int FOOTER_SIZE = 4;   // crc32(4)

QByteArray MessageFramer::frameMessage(MessageType type, const QByteArray& payload) {
    QByteArray frame;
    QDataStream stream(&frame, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    quint32 payloadLen = static_cast<quint32>(payload.size());

    // Write header
    stream << PROTOCOL_MAGIC << payloadLen << static_cast<quint32>(type);

    // Write payload
    stream.writeRawData(payload.constData(), payload.size());

    // Compute CRC32 over (type_bytes || payload)
    QByteArray typeAndPayload;
    QDataStream tpStream(&typeAndPayload, QIODevice::WriteOnly);
    tpStream.setByteOrder(QDataStream::BigEndian);
    tpStream << static_cast<quint32>(type);
    tpStream.writeRawData(payload.constData(), payload.size());

    quint32 crc = Checksum::crc32(typeAndPayload);
    stream << crc;

    return frame;
}

void MessageFramer::feedData(const QByteArray& data) {
    m_buffer.append(data);
}

FrameResult MessageFramer::tryParseFrame(MessageType& outType, QByteArray& outPayload) {
    // Need at least header
    if (static_cast<quint32>(m_buffer.size()) < HEADER_SIZE)
        return FrameResult::Incomplete;

    QDataStream stream(m_buffer);
    stream.setByteOrder(QDataStream::BigEndian);

    quint32 magic = 0, payloadLen = 0, typeVal = 0;
    stream >> magic >> payloadLen >> typeVal;

    // Validate magic
    if (magic != PROTOCOL_MAGIC) {
        // Discard one byte and re-sync
        m_buffer.remove(0, 1);
        return FrameResult::Invalid;
    }

    // Guard against unreasonable payload length
    if (payloadLen > 128 * 1024 * 1024) { // 128 MB sanity cap
        m_buffer.remove(0, 1);
        return FrameResult::Invalid;
    }

    quint32 totalFrameSize = HEADER_SIZE + payloadLen + FOOTER_SIZE;
    if (static_cast<quint32>(m_buffer.size()) < totalFrameSize)
        return FrameResult::Incomplete;

    // Extract payload
    char* rawBuf = m_buffer.data();
    QByteArray payload(rawBuf + HEADER_SIZE, static_cast<int>(payloadLen));

    // Read CRC32 from footer
    QByteArray footerBytes(rawBuf + HEADER_SIZE + payloadLen, FOOTER_SIZE);
    QDataStream footerStream(footerBytes);
    footerStream.setByteOrder(QDataStream::BigEndian);
    quint32 receivedCrc = 0;
    footerStream >> receivedCrc;

    // Verify CRC32 over (type || payload)
    QByteArray typeAndPayload;
    QDataStream tpStream(&typeAndPayload, QIODevice::WriteOnly);
    tpStream.setByteOrder(QDataStream::BigEndian);
    tpStream << typeVal;
    tpStream.writeRawData(payload.constData(), payload.size());
    quint32 computedCrc = Checksum::crc32(typeAndPayload);

    if (receivedCrc != computedCrc) {
        m_buffer.remove(0, totalFrameSize);
        return FrameResult::Invalid;
    }

    // Success
    outType = static_cast<MessageType>(typeVal);
    outPayload = payload;
    m_buffer.remove(0, totalFrameSize);
    return FrameResult::Complete;
}
