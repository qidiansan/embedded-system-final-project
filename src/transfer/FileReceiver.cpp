#include "FileReceiver.h"
#include "network/PeerConnection.h"
#include "utils/Checksum.h"
#include <QDir>
#include <QFileInfo>

FileReceiver::FileReceiver(QObject* parent)
    : QObject(parent)
{
}

void FileReceiver::setSaveRoot(const QString& root) {
    m_saveRoot = root;
}

void FileReceiver::cancel() {
    m_active = false;
    if (m_file.isOpen()) {
        m_file.close();
        m_file.remove();  // delete partial file
    }
}

void FileReceiver::onFileInfo(const FileInfo& info) {
    m_currentInfo = info;
    m_received = 0;
    m_active = true;

    // Build full path
    QString fullPath = m_saveRoot + "/" + info.relativePath;

    // Ensure parent directory exists
    QDir dir = QFileInfo(fullPath).absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    m_file.setFileName(fullPath);
    if (!m_file.open(QIODevice::WriteOnly)) {
        emit error(info.relativePath, "Cannot create file: " + m_file.errorString());
        m_active = false;
        return;
    }

    emit logMessage(QString("Receiving: %1 (%2 bytes, %3 chunks)")
        .arg(info.relativePath).arg(info.fileSize).arg(info.totalChunks));
}

void FileReceiver::onFileChunk(quint32 index, const QByteArray& data) {
    if (!m_active) return;

    qint64 offset = static_cast<qint64>(index) * m_currentInfo.chunkSize;
    if (!m_file.seek(offset)) {
        emit error(m_currentInfo.relativePath, "Seek failed in output file");
        m_active = false;
        return;
    }
    if (m_file.write(data) != data.size()) {
        emit error(m_currentInfo.relativePath, "Write failed: " + m_file.errorString());
        m_active = false;
        return;
    }

    m_received += data.size();
    emit progress(m_currentInfo.relativePath, m_received,
                  m_currentInfo.fileSize > 0 ? m_currentInfo.fileSize : 0);
}

void FileReceiver::onFileComplete(const QString& relativePath, const QString& fileMD5) {
    Q_UNUSED(relativePath)
    if (!m_active) return;

    m_file.close();

    // Verify whole-file MD5
    QByteArray actual = Checksum::md5File(m_file.fileName());
    if (actual.toHex() != fileMD5.toLatin1()) {
        emit error(m_currentInfo.relativePath,
                   QString("MD5 mismatch! Expected: %1, Got: %2")
                       .arg(fileMD5, QString::fromLatin1(actual.toHex())));
        m_file.remove();
        m_active = false;
        return;
    }

    m_active = false;
    emit logMessage(QString("File complete (MD5 OK): %1").arg(m_currentInfo.relativePath));
    emit fileComplete(m_currentInfo.relativePath, m_file.fileName());
}

void FileReceiver::onCancel() {
    cancel();
}
