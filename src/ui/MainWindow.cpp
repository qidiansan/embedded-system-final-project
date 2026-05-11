#include "MainWindow.h"
#include "ConnectionPanel.h"
#include "TransferPanel.h"
#include "LogPanel.h"
#include "network/PeerServer.h"
#include "network/PeerClient.h"
#include "network/PeerConnection.h"
#include "network/TransferManager.h"
#include <QSplitter>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_server(new PeerServer(this))
    , m_client(new PeerClient(this))
    , m_transferMgr(new TransferManager(this))
{
    setupUi();

    // Server signals
    connect(m_server, &PeerServer::clientConnected,
            this, &MainWindow::onClientConnected);
    connect(m_server, &PeerServer::logMessage,
            m_logPanel, &LogPanel::appendLog);
    connect(m_server, &PeerServer::serverError,
            this, [this](const QString& e) {
                m_logPanel->appendLog("Server error: " + e);
                m_connPanel->setStatus("Error: " + e);
                m_connPanel->setConnected(false);
            });

    // Client signals
    connect(m_client, &PeerClient::connected,
            this, &MainWindow::onClientConnectedToServer);
    connect(m_client, &PeerClient::logMessage,
            m_logPanel, &LogPanel::appendLog);
    connect(m_client, &PeerClient::connectionError,
            this, &MainWindow::onConnectionError);

    // TransferManager signals
    connect(m_transferMgr, &TransferManager::logMessage,
            m_logPanel, &LogPanel::appendLog);
    connect(m_transferMgr, &TransferManager::transferProgress,
            this, &MainWindow::onTransferProgress);
    connect(m_transferMgr, &TransferManager::transferComplete,
            this, &MainWindow::onTransferComplete);
    connect(m_transferMgr, &TransferManager::transferError,
            this, &MainWindow::onTransferError);

    // Set default save root to a subfolder next to the executable
    QString defaultSave = QDir::currentPath() + "/received_files";
    m_transferMgr->setSaveRoot(defaultSave);
}

MainWindow::~MainWindow() {
    if (m_server->isListening()) m_server->stop();
    if (m_client->isConnected()) m_client->disconnect();
}

void MainWindow::setupUi() {
    setWindowTitle("Network File Transfer Tool");
    resize(900, 620);

    auto* central = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // Connection panel at top
    m_connPanel = new ConnectionPanel(central);
    mainLayout->addWidget(m_connPanel);

    // Transfer + Log splitter
    auto* splitter = new QSplitter(Qt::Horizontal, central);
    m_transferPanel = new TransferPanel(splitter);
    m_logPanel = new LogPanel(splitter);
    splitter->addWidget(m_transferPanel);
    splitter->addWidget(m_logPanel);
    splitter->setStretchFactor(0, 7);
    splitter->setStretchFactor(1, 3);
    mainLayout->addWidget(splitter, 1);

    setCentralWidget(central);

    // Connect ConnectionPanel signals
    connect(m_connPanel, &ConnectionPanel::startServerRequested,
            this, &MainWindow::onStartServer);
    connect(m_connPanel, &ConnectionPanel::connectRequested,
            this, &MainWindow::onConnect);
    connect(m_connPanel, &ConnectionPanel::stopRequested,
            this, &MainWindow::onStop);

    // Connect TransferPanel signals
    connect(m_transferPanel, &TransferPanel::sendFileRequested,
            this, &MainWindow::onSendFile);
    connect(m_transferPanel, &TransferPanel::sendDirRequested,
            this, &MainWindow::onSendDir);

    // Status bar
    statusBar()->showMessage("Ready");
}

void MainWindow::setupPeerConnection(QTcpSocket* socket) {
    m_peerConn = new PeerConnection(socket, this);
    connect(m_peerConn, &PeerConnection::handshakeReceived,
            this, &MainWindow::onHandshake);
    connect(m_peerConn, &PeerConnection::disconnected,
            this, &MainWindow::onPeerDisconnected);
    connect(m_peerConn, &PeerConnection::logMessage,
            m_logPanel, &LogPanel::appendLog);

    m_transferMgr->setPeerConnection(m_peerConn);

    // Send handshake
    m_peerConn->sendHandshake(QSysInfo::machineHostName());
}

// --- Server mode: listening ---
void MainWindow::onStartServer(quint16 port) {
    if (m_server->startListening(port)) {
        m_connPanel->setStatus(QString("Listening on port %1").arg(port));
        m_connPanel->setConnected(true);
        statusBar()->showMessage(QString("Server mode — port %1").arg(port));
        // When a client already exists and is connected, disconnect it
        if (m_client->isConnected()) {
            m_client->disconnect();
        }
    }
}

// --- Client mode: connecting ---
void MainWindow::onConnect(const QString& host, quint16 port) {
    m_client->connectToServer(host, port);
}

// --- Server accepted a client ---
void MainWindow::onClientConnected(QTcpSocket* socket) {
    m_logPanel->appendLog("Client connected, performing handshake...");
    setupPeerConnection(socket);
}

// --- Client connected to server ---
void MainWindow::onClientConnectedToServer(QTcpSocket* socket) {
    m_connPanel->setStatus(QString("Connected to %1").arg(socket->peerAddress().toString()));
    m_connPanel->setConnected(true);
    statusBar()->showMessage(QString("Client mode — connected to %1").arg(socket->peerAddress().toString()));
    setupPeerConnection(socket);
}

void MainWindow::onConnectionError(const QString& error) {
    m_logPanel->appendLog("Connection error: " + error);
    m_connPanel->setStatus("Error: " + error);
    m_connPanel->setConnected(false);
}

// --- Handshake complete ---
void MainWindow::onHandshake(const QString& version, const QString& hostname) {
    m_logPanel->appendLog(QString("Handshake OK: %1 (v%2)").arg(hostname, version));
    statusBar()->showMessage(QString("Connected to %1").arg(hostname));
}

// --- Stop ---
void MainWindow::onStop() {
    if (m_server->isListening()) m_server->stop();
    if (m_client->isConnected()) m_client->disconnect();
    m_connPanel->setStatus("Disconnected");
    m_connPanel->setConnected(false);
    statusBar()->showMessage("Disconnected");
}

void MainWindow::onPeerDisconnected() {
    m_connPanel->setStatus("Disconnected");
    m_connPanel->setConnected(false);
    statusBar()->showMessage("Peer disconnected");
}

// --- Send ---
void MainWindow::onSendFile(const QString& path) {
    QFileInfo fi(path);
    int row = m_transferPanel->addTransfer(fi.fileName(), true, fi.size());
    m_transferRows[path] = row;
    m_transferMgr->enqueueSend(path);
}

void MainWindow::onSendDir(const QString& path) {
    QFileInfo fi(path);
    int row = m_transferPanel->addTransfer(fi.fileName() + "/", true, 0);
    m_transferRows[path] = row;
    m_transferMgr->enqueueSend(path);
}

// --- Progress ---
void MainWindow::onTransferProgress(const QString& fileName, qint64 transferred, qint64 total) {
    Q_UNUSED(fileName)
    // Update the current active row
    for (auto it = m_transferRows.begin(); it != m_transferRows.end(); ++it) {
        int row = it.value();
        if (total > 0) {
            int pct = static_cast<int>(transferred * 100 / total);
            m_transferPanel->updateProgress(row, pct);
        }
    }
    statusBar()->showMessage(
        QString("Transferring... %1 / %2 bytes")
            .arg(transferred).arg(total));
}

void MainWindow::onTransferComplete(const QString& fileName) {
    m_logPanel->appendLog("Transfer complete: " + fileName);
    // Find and update the row
    for (auto it = m_transferRows.begin(); it != m_transferRows.end(); ++it) {
        if (it.key().endsWith(fileName) || fileName.endsWith(QFileInfo(it.key()).fileName())) {
            m_transferPanel->updateProgress(it.value(), 100);
            m_transferPanel->setStatus(it.value(), "Complete");
            break;
        }
    }
    statusBar()->showMessage("Transfer complete: " + fileName, 5000);
}

void MainWindow::onTransferError(const QString& fileName, const QString& error) {
    m_logPanel->appendLog(QString("ERROR: %1 — %2").arg(fileName, error));
    for (auto it = m_transferRows.begin(); it != m_transferRows.end(); ++it) {
        if (it.key().endsWith(fileName) || fileName.endsWith(QFileInfo(it.key()).fileName())) {
            m_transferPanel->setStatus(it.value(), "Error: " + error);
            break;
        }
    }
}
