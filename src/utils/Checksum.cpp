#include "Checksum.h"
#include <QCryptographicHash>
#include <QFile>

QByteArray Checksum::md5File(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    QCryptographicHash hash(QCryptographicHash::Md5);
    constexpr qint64 BUF_SIZE = 4 * 1024 * 1024; // 4 MB read buffer
    while (!file.atEnd()) {
        hash.addData(file.read(BUF_SIZE));
    }
    return hash.result();  // raw 16 bytes, callers call .toHex() when needed
}

QByteArray Checksum::md5Data(const QByteArray& data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Md5);
}

quint32 Checksum::crc32(const QByteArray& data) {
    // Software CRC32 implementation (IEEE 802.3 polynomial)
    static quint32 table[256];
    static bool initialized = false;
    if (!initialized) {
        for (quint32 i = 0; i < 256; i++) {
            quint32 crc = i;
            for (int j = 0; j < 8; j++) {
                crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320u : 0u);
            }
            table[i] = crc;
        }
        initialized = true;
    }

    quint32 crc = 0xFFFFFFFFu;
    for (int i = 0; i < data.size(); i++) {
        crc = table[(crc ^ static_cast<quint8>(data[i])) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}
