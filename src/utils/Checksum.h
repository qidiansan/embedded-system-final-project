#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <QByteArray>
#include <QString>

class Checksum {
public:
    static QByteArray md5File(const QString& filePath);
    static QByteArray md5Data(const QByteArray& data);
    static quint32   crc32(const QByteArray& data);
};

#endif // CHECKSUM_H
