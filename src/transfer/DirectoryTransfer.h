#ifndef DIRECTORYTRANSFER_H
#define DIRECTORYTRANSFER_H

#include "protocol/ProtocolDefs.h"
#include <QObject>
#include <QString>
#include <QVector>

class PeerConnection;
class FileSender;
class FileReceiver;

class DirScanEntry {
public:
    QString relativePath;  // relative to scanned root
    bool    isDir;
    qint64  fileSize;      // 0 for directories
};

class DirectoryTransfer : public QObject {
    Q_OBJECT
public:
    explicit DirectoryTransfer(QObject* parent = nullptr);

    // Scan a local directory, returning entries sorted (dirs first, then files)
    static QVector<DirScanEntry> scanDirectory(const QString& dirPath);

    // Send a directory recursively via PeerConnection
    void sendDirectory(PeerConnection* conn, const QString& localDirPath);

    // Receive: the receiver just uses FileReceiver — dir creation is handled
    // by onDirCreateReceived signal

signals:
    void progress(const QString& fileName, qint64 sent, qint64 total);
    void logMessage(const QString& msg);
    void allComplete();

private slots:
    void onFileComplete(const QString& fileName);
    void onFileError(const QString& fileName, const QString& errMsg);

private:
    void sendNextFile();

    PeerConnection*     m_conn = nullptr;
    FileSender*         m_fileSender = nullptr;
    QVector<DirScanEntry> m_entries;
    QString             m_localRoot;
    int                 m_currentIndex = 0;
    qint64              m_totalBytes = 0;
    qint64              m_sentBytes = 0;
    int                 m_completedCount = 0;
    bool                m_cancelled = false;
};

#endif // DIRECTORYTRANSFER_H
