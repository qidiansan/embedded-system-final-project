#include "DirectoryTransfer.h"
#include "FileSender.h"
#include "network/PeerConnection.h"
#include <QDirIterator>
#include <QFileInfo>

DirectoryTransfer::DirectoryTransfer(QObject* parent)
    : QObject(parent)
{
}

QVector<DirScanEntry> DirectoryTransfer::scanDirectory(const QString& dirPath) {
    QVector<DirScanEntry> entries;
    QDirIterator it(dirPath, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        it.next();
        QFileInfo fi = it.fileInfo();

        // Compute path relative to scanned root
        QString relativePath = QDir(dirPath).relativeFilePath(fi.absoluteFilePath());

        DirScanEntry entry;
        entry.relativePath = relativePath;
        entry.isDir        = fi.isDir();
        entry.fileSize     = fi.isDir() ? 0 : fi.size();
        entries.append(entry);
    }

    // Sort: directories first (so they get created before files inside them),
    // then files. Within each group, sort by path depth.
    std::sort(entries.begin(), entries.end(),
              [](const DirScanEntry& a, const DirScanEntry& b) {
                  if (a.isDir != b.isDir) return a.isDir; // dirs before files
                  return a.relativePath < b.relativePath;
              });

    return entries;
}

void DirectoryTransfer::sendDirectory(PeerConnection* conn, const QString& localDirPath) {
    m_conn = conn;
    m_cancelled = false;
    m_currentIndex = 0;
    m_completedCount = 0;
    m_sentBytes = 0;

    QFileInfo fi(localDirPath);
    if (!fi.isDir()) {
        emit logMessage("Error: not a directory: " + localDirPath);
        emit allComplete();
        return;
    }

    m_entries = scanDirectory(localDirPath);
    if (m_entries.isEmpty()) {
        emit logMessage("Empty directory, nothing to send.");
        emit allComplete();
        return;
    }

    // Compute total bytes
    m_totalBytes = 0;
    for (const auto& e : m_entries) {
        if (!e.isDir) m_totalBytes += e.fileSize;
    }

    QString dirName = fi.dir().dirName().isEmpty() ? fi.fileName() : fi.fileName();
    emit logMessage(QString("Sending directory '%1': %2 entries, %3 bytes")
        .arg(dirName).arg(m_entries.size()).arg(m_totalBytes));

    // First pass: send DIR_CREATE for all directories
    for (const auto& e : m_entries) {
        if (e.isDir) {
            conn->sendDirCreate(e.relativePath);
        }
    }

    // Create a FileSender for sequential file sending
    m_fileSender = new FileSender(this);
    connect(m_fileSender, &FileSender::fileComplete, this, &DirectoryTransfer::onFileComplete);
    connect(m_fileSender, &FileSender::error, this, &DirectoryTransfer::onFileError);
    connect(m_fileSender, &FileSender::progress, [this](const QString&, qint64 sent, qint64 total) {
        Q_UNUSED(total)
        m_sentBytes += sent;
        emit progress(m_entries.at(m_currentIndex).relativePath, m_sentBytes, m_totalBytes);
    });

    // Start sending the first file
    m_localRoot = localDirPath;
    sendNextFile();
}

void DirectoryTransfer::sendNextFile() {
    while (m_currentIndex < m_entries.size() && m_entries[m_currentIndex].isDir) {
        m_currentIndex++;
    }

    if (m_currentIndex >= m_entries.size()) {
        m_conn->sendTransferComplete();
        emit allComplete();
        return;
    }

    const DirScanEntry& entry = m_entries[m_currentIndex];
    QString fullPath = m_localRoot + "/" + entry.relativePath;
    // remoteRoot is the directory name, stripping the local prefix
    QString remoteDir = QFileInfo(m_localRoot).fileName();
    m_fileSender->start(m_conn, fullPath, remoteDir);
}

void DirectoryTransfer::onFileComplete(const QString& /*fileName*/) {
    m_completedCount++;
    m_currentIndex++;
    sendNextFile();
}

void DirectoryTransfer::onFileError(const QString& fileName, const QString& errMsg) {
    emit logMessage(QString("Error sending %1: %2").arg(fileName, errMsg));
    m_currentIndex++;
    sendNextFile();
}
