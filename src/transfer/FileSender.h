#ifndef FILESENDER_H
#define FILESENDER_H

#include "protocol/ProtocolDefs.h"
#include <QObject>
#include <QFile>
#include <QString>

class PeerConnection;

class FileSender : public QObject {
    Q_OBJECT
public:
    explicit FileSender(QObject* parent = nullptr);

    void start(PeerConnection* conn, const QString& filePath, const QString& remoteRoot = QString());
    void cancel();
    bool isActive() const { return m_active; }

signals:
    void progress(const QString& fileName, qint64 sent, qint64 total);
    void fileComplete(const QString& fileName);
    void error(const QString& fileName, const QString& errMsg);
    void logMessage(const QString& msg);

private slots:
    void sendNextChunk();

private:
    PeerConnection* m_conn = nullptr;
    QFile          m_file;
    QString        m_relativePath;
    qint64         m_fileSize = 0;
    quint32        m_chunkSize = DEFAULT_CHUNK_SIZE;
    quint32        m_totalChunks = 0;
    quint32        m_currentChunk = 0;
    QByteArray     m_fileMD5;
    bool           m_active = false;
    bool           m_cancelled = false;
};

#endif // FILESENDER_H
