#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>

class ConnectionPanel;
class TransferPanel;
class LogPanel;
class PeerServer;
class PeerClient;
class PeerConnection;
class TransferManager;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onStartServer(quint16 port);
    void onConnect(const QString& host, quint16 port);
    void onStop();
    void onClientConnected(QTcpSocket* socket);
    void onClientConnectedToServer(QTcpSocket* socket);
    void onConnectionError(const QString& error);
    void onHandshake(const QString& version, const QString& hostname);
    void onPeerDisconnected();
    void onSendFile(const QString& path);
    void onSendDir(const QString& path);
    void onTransferProgress(const QString& fileName, qint64 transferred, qint64 total);
    void onTransferComplete(const QString& fileName);
    void onTransferError(const QString& fileName, const QString& error);

private:
    void setupUi();
    void setupPeerConnection(QTcpSocket* socket);

    ConnectionPanel*  m_connPanel;
    TransferPanel*    m_transferPanel;
    LogPanel*         m_logPanel;

    PeerServer*       m_server;
    PeerClient*       m_client;
    PeerConnection*   m_peerConn = nullptr;
    TransferManager*  m_transferMgr;

    // Track transfer table rows
    QMap<QString, int> m_transferRows;
};

#endif // MAINWINDOW_H
