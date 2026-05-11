#ifndef MESSAGEFRAMER_H
#define MESSAGEFRAMER_H

#include "ProtocolDefs.h"
#include <QByteArray>

enum class FrameResult {
    Complete,
    Incomplete,
    Invalid
};

class MessageFramer {
public:
    void feedData(const QByteArray& data);    // append raw bytes from socket

    // Attempt to extract ONE complete frame. Returns Complete/Incomplete/Invalid.
    // On Complete, outType + outPayload are filled and the frame is removed from buffer.
    FrameResult tryParseFrame(MessageType& outType, QByteArray& outPayload);

    static QByteArray frameMessage(MessageType type, const QByteArray& payload);

    qint64 bufferSize() const { return m_buffer.size(); }

private:
    QByteArray m_buffer;
};

#endif // MESSAGEFRAMER_H
