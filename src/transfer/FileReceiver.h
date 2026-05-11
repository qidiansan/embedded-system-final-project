#ifndef FILERECEIVER_H
#define FILERECEIVER_H

#include "protocol/ProtocolDefs.h"
#include <QObject>
#include <QFile>
#include <QString>

class PeerConnection;

class FileReceiver : public QObject {
    Q_OBJECT
public:
    explicit FileReceiver(QObject* parent = nullptr);

    void setSaveRoot(const QString& root);
    QString saveRoot() const { return m_saveRoot; }
    bool isActive() const { return m_active; }
    void cancel();

signals:
    void progress(const QString& fileName, qint64 received, qint64 total);
    void fileComplete(const QString& fileName, const QString& fullPath);
    void error(const QString& fileName, const QString& errMsg);
    void logMessage(const QString& msg);

public slots:
    void onFileInfo(const FileInfo& info);
    void onFileChunk(quint32 index, const QByteArray& data);
    void onFileComplete(const QString& relativePath, const QString& fileMD5);
    void onCancel();

private:
    PeerConnection* m_conn    = nullptr;
    QString         m_saveRoot;
    QFile           m_file;
    FileInfo        m_currentInfo;
    qint64          m_received = 0;
    bool            m_active   = false;
};

#endif // FILERECEIVER_H
