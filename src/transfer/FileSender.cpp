#include "FileSender.h"
#include "network/PeerConnection.h"
#include "utils/Checksum.h"
#include <QFileInfo>

FileSender::FileSender(QObject* parent)
    : QObject(parent)
{
}

void FileSender::start(PeerConnection* conn, const QString& filePath, const QString& remoteRoot) {
    m_conn = conn;
    m_currentChunk = 0;
    m_cancelled = false;

    QFileInfo fi(filePath);
    if (!fi.exists()) {
        emit error(fi.fileName(), "File not found: " + filePath);
        m_active = false;
        return;
    }

    m_fileSize = fi.size();
    m_relativePath = remoteRoot.isEmpty() ? fi.fileName()
                    : remoteRoot + "/" + fi.fileName();

    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::ReadOnly)) {
        emit error(fi.fileName(), "Cannot open file: " + m_file.errorString());
        m_active = false;
        return;
    }

    // Compute MD5 of whole file (streaming, won't block too long for reasonable files)
    m_fileMD5 = Checksum::md5File(filePath);
    m_file.seek(0);

    m_totalChunks = (m_fileSize == 0)
        ? 1  // send one empty chunk for 0-byte files
        : static_cast<quint32>((m_fileSize + m_chunkSize - 1) / m_chunkSize);

    emit logMessage(QString("Sending: %1 (%2 bytes, %3 chunks)")
        .arg(m_relativePath).arg(m_fileSize).arg(m_totalChunks));

    // Send file info
    FileInfo info;
    info.relativePath = m_relativePath;
    info.fileSize     = m_fileSize;
    info.fileMD5      = QString::fromLatin1(m_fileMD5.toHex());
    info.chunkSize    = m_chunkSize;
    info.totalChunks  = m_totalChunks;
    m_conn->sendFileInfo(info);

    m_active = true;
    sendNextChunk();
}

void FileSender::cancel() {
    m_cancelled = true;
    m_active = false;
    m_file.close();
}

void FileSender::sendNextChunk() {
    if (m_cancelled || !m_active) return;

    if (m_currentChunk >= m_totalChunks) {
        // All chunks sent
        m_active = false;
        m_file.close();
        m_conn->sendFileComplete(m_relativePath,
                                 QString::fromLatin1(m_fileMD5.toHex()));
        emit fileComplete(m_relativePath);
        return;
    }

    QByteArray data = m_file.read(m_chunkSize);
    m_conn->sendFileChunk(m_currentChunk, data);
    emit progress(m_relativePath,
                  static_cast<qint64>(m_currentChunk) * m_chunkSize + data.size(),
                  m_fileSize > 0 ? m_fileSize : 0);

    m_currentChunk++;

    // Process next chunk on next event loop iteration (keeps GUI responsive)
    QMetaObject::invokeMethod(this, "sendNextChunk", Qt::QueuedConnection);
}
