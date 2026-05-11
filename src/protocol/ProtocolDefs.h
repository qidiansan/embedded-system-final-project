#ifndef PROTOCOLDEFS_H
#define PROTOCOLDEFS_H

#include <QtGlobal>
#include <QString>

constexpr quint32 PROTOCOL_MAGIC  = 0x4E465432;   // "NFT2"
constexpr quint16 DEFAULT_PORT    = 9999;
constexpr quint32 DEFAULT_CHUNK_SIZE = 262144;     // 256 KB
constexpr quint32 MAX_CHUNK_SIZE  = 1048576;       // 1 MB cap
constexpr const char* PROTOCOL_VERSION = "1.0";

enum class MessageType : quint32 {
    MSG_HANDSHAKE_REQ  = 0x01,
    MSG_HANDSHAKE_ACK  = 0x02,
    MSG_FILE_INFO       = 0x03,
    MSG_FILE_CHUNK      = 0x04,
    MSG_FILE_COMPLETE   = 0x05,
    MSG_DIR_CREATE      = 0x08,
    MSG_TRANSFER_COMPLETE = 0x09,
    MSG_ERROR           = 0x0A,
    MSG_FILE_REQUEST    = 0x0B,
    MSG_CANCEL          = 0x0C
};

enum class TransferError : quint32 {
    ERR_NONE            = 0,
    ERR_CHECKSUM_MISMATCH = 1,
    ERR_FILE_NOT_FOUND  = 2,
    ERR_DISK_FULL       = 3,
    ERR_PROTOCOL        = 4,
    ERR_CONNECTION      = 5,
    ERR_UNKNOWN         = 99
};

struct FileInfo {
    QString relativePath;
    qint64  fileSize;
    QString fileMD5;
    quint32 chunkSize;
    quint32 totalChunks;
};

struct DirEntry {
    QString name;
    bool    isDirectory;
    qint64  size;
};

#endif // PROTOCOLDEFS_H
