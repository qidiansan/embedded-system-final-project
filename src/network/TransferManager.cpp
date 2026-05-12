#include "TransferManager.h"
#include "PeerConnection.h"
#include "transfer/FileSender.h"
#include "transfer/FileReceiver.h"
#include "transfer/DirectoryTransfer.h"
#include <QFileInfo>
#include <QDir>

TransferManager::TransferManager(QObject* parent)
    : QObject(parent)
{
}

void TransferManager::setPeerConnection(PeerConnection* conn) {
    m_conn = conn;
    if (!m_conn) return;

    // Wire up incoming messages to receiver
    connect(m_conn, &PeerConnection::fileInfoReceived,
            this, &TransferManager::onFileInfoReceived);
    connect(m_conn, &PeerConnection::chunkReceived,
            this, &TransferManager::onChunkReceived);
    connect(m_conn, &PeerConnection::fileCompleteReceived,
            this, &TransferManager::onFileCompleteReceived);
    connect(m_conn, &PeerConnection::dirCreateReceived,
            this, &TransferManager::onDirCreateReceived);
    connect(m_conn, &PeerConnection::cancelReceived,
            this, &TransferManager::onCancelReceived);
    connect(m_conn, &PeerConnection::logMessage,
            this, &TransferManager::logMessage);
}

void TransferManager::setSaveRoot(const QString& root) {
    m_saveRoot = root;
}

void TransferManager::enqueueSend(const QString& localPath, const QString& remotePath) {
    QFileInfo fi(localPath);
    TransferJob job;
    job.localPath  = localPath;
    job.remotePath = remotePath;
    job.jobType    = fi.isDir() ? TransferJob::Type::Directory
                                : TransferJob::Type::File;

    m_queue.enqueue(job);
    m_totalJobs++;
    m_pendingJobs = m_queue.size();
    emit queueUpdated(m_pendingJobs, m_totalJobs);
    emit logMessage(QString("Queued: %1").arg(localPath));

    if (!isBusy()) {
        startNextJob();
    }
}

void TransferManager::cancelCurrent() {
    if (m_fileSender && m_fileSender->isActive()) {
        m_fileSender->cancel();
        m_conn->sendCancel();
    }
    if (m_fileReceiver && m_fileReceiver->isActive()) {
        m_fileReceiver->cancel();
    }
}

void TransferManager::cancelAll() {
    m_queue.clear();
    m_pendingJobs = 0;
    cancelCurrent();
    emit queueUpdated(0, m_totalJobs);
}

bool TransferManager::isBusy() const {
    return (m_fileSender && m_fileSender->isActive())
        || (m_fileReceiver && m_fileReceiver->isActive())
        || (m_dirTransfer != nullptr);
}

// --- Private: start jobs ---
void TransferManager::startNextJob() {
    if (m_queue.isEmpty()) {
        m_sending = false;
        emit logMessage("All transfers complete.");
        return;
    }

    TransferJob job = m_queue.dequeue();
    m_pendingJobs = m_queue.size();
    emit queueUpdated(m_pendingJobs, m_totalJobs);
    m_sending = true;

    if (job.jobType == TransferJob::Type::Directory) {
        startDirSend(job.localPath, job.remotePath);
    } else {
        startFileSend(job.localPath, job.remotePath);
    }
}

void TransferManager::startFileSend(const QString& filePath, const QString& remoteRoot) {
    if (!m_fileSender) {
        m_fileSender = new FileSender(this);
        connect(m_fileSender, &FileSender::fileComplete,
                this, &TransferManager::onFileSentComplete);
        connect(m_fileSender, &FileSender::error,
                this, &TransferManager::onFileSentError);
        connect(m_fileSender, &FileSender::progress,
                this, &TransferManager::transferProgress);
        connect(m_fileSender, &FileSender::logMessage,
                this, &TransferManager::logMessage);
    }
    m_fileSender->start(m_conn, filePath, remoteRoot);
}

void TransferManager::startDirSend(const QString& dirPath, const QString& remoteRoot) {
    Q_UNUSED(remoteRoot)
    m_currentDirName = QFileInfo(dirPath).fileName() + "/";
    if (!m_dirTransfer) {
        m_dirTransfer = new DirectoryTransfer(this);
        connect(m_dirTransfer, &DirectoryTransfer::allComplete,
                this, &TransferManager::onDirAllComplete);
        connect(m_dirTransfer, &DirectoryTransfer::logMessage,
                this, &TransferManager::logMessage);
        connect(m_dirTransfer, &DirectoryTransfer::progress,
                this, &TransferManager::transferProgress);
    }
    m_dirTransfer->sendDirectory(m_conn, dirPath);
}

// --- Private: receive ---
void TransferManager::onFileInfoReceived(const FileInfo& info) {
    if (!m_fileReceiver) {
        m_fileReceiver = new FileReceiver(this);
        m_fileReceiver->setSaveRoot(m_saveRoot);
        connect(m_fileReceiver, &FileReceiver::progress,
                this, &TransferManager::transferProgress);
        connect(m_fileReceiver, &FileReceiver::fileComplete,
                this, [this](const QString& name, const QString&) {
                    emit transferComplete(name);
                });
        connect(m_fileReceiver, &FileReceiver::error,
                this, &TransferManager::transferError);
        connect(m_fileReceiver, &FileReceiver::logMessage,
                this, &TransferManager::logMessage);
    }
    m_fileReceiver->setSaveRoot(m_saveRoot); // update in case it changed
    m_fileReceiver->onFileInfo(info);
}

void TransferManager::onChunkReceived(quint32 index, const QByteArray& data) {
    if (m_fileReceiver)
        m_fileReceiver->onFileChunk(index, data);
}

void TransferManager::onFileCompleteReceived(const QString& relativePath, const QString& fileMD5) {
    if (m_fileReceiver)
        m_fileReceiver->onFileComplete(relativePath, fileMD5);
}

void TransferManager::onDirCreateReceived(const QString& relativeDirPath) {
    QString fullPath = m_saveRoot + "/" + relativeDirPath;
    QDir().mkpath(fullPath);
    emit logMessage(QString("Created directory: %1").arg(relativeDirPath));
}

void TransferManager::onCancelReceived() {
    if (m_fileReceiver)
        m_fileReceiver->onCancel();
    emit logMessage("Transfer cancelled by peer.");
}

// --- Private: send-completion slots ---
void TransferManager::onFileSentComplete(const QString& fileName) {
    emit transferComplete(fileName);
    m_sending = false;
    startNextJob();
}

void TransferManager::onFileSentError(const QString& fileName, const QString& errMsg) {
    emit transferError(fileName, errMsg);
    m_sending = false;
    startNextJob();
}

void TransferManager::onDirAllComplete() {
    emit logMessage("Directory transfer complete.");
    emit directoryComplete(m_currentDirName);
    m_currentDirName.clear();
    m_sending = false;
    if (m_dirTransfer) {
        m_dirTransfer->deleteLater();
        m_dirTransfer = nullptr;
    }
    startNextJob();
}
