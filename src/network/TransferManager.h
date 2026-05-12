#ifndef TRANSFERMANAGER_H
#define TRANSFERMANAGER_H

#include <QObject>
#include <QQueue>
#include "protocol/ProtocolDefs.h"

class PeerConnection;
class FileSender;
class FileReceiver;
class DirectoryTransfer;

struct TransferJob {
    enum class Type { File, Directory };
    Type    jobType;
    QString localPath;
    QString remotePath;   // optional, root directory on remote
};

class TransferManager : public QObject {
    Q_OBJECT
public:
    explicit TransferManager(QObject* parent = nullptr);

    void setPeerConnection(PeerConnection* conn);
    PeerConnection* peerConnection() const { return m_conn; }

    void setSaveRoot(const QString& root);
    QString saveRoot() const { return m_saveRoot; }

    void enqueueSend(const QString& localPath, const QString& remotePath = QString());
    void cancelCurrent();
    void cancelAll();

    bool isBusy() const;

signals:
    void transferProgress(const QString& fileName, qint64 transferred, qint64 total);
    void transferComplete(const QString& fileName);
    void transferError(const QString& fileName, const QString& error);
    void directoryComplete(const QString& dirName);
    void queueUpdated(int pending, int total);
    void logMessage(const QString& msg);

private slots:
    void onFileInfoReceived(const FileInfo& info);
    void onChunkReceived(quint32 index, const QByteArray& data);
    void onFileCompleteReceived(const QString& relativePath, const QString& fileMD5);
    void onDirCreateReceived(const QString& relativeDirPath);
    void onFileSentComplete(const QString& fileName);
    void onFileSentError(const QString& fileName, const QString& errMsg);
    void onDirAllComplete();
    void onCancelReceived();

private:
    void startNextJob();
    void startFileSend(const QString& filePath, const QString& remoteRoot);
    void startDirSend(const QString& dirPath, const QString& remoteRoot);

    PeerConnection*     m_conn = nullptr;
    FileSender*         m_fileSender = nullptr;
    FileReceiver*       m_fileReceiver = nullptr;
    DirectoryTransfer*  m_dirTransfer = nullptr;
    QQueue<TransferJob> m_queue;
    QString             m_saveRoot;
    QString             m_currentDirName;
    bool                m_sending = false;
    int                 m_totalJobs = 0;
    int                 m_pendingJobs = 0;
};

#endif // TRANSFERMANAGER_H
