#include "ConnectionPanel.h"
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>

ConnectionPanel::ConnectionPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* group = new QGroupBox("Connection", this);
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(group);

    auto* layout = new QHBoxLayout(group);

    // Mode selector
    layout->addWidget(new QLabel("Mode:", group));
    m_modeCombo = new QComboBox(group);
    m_modeCombo->addItem("Server (Listen)");
    m_modeCombo->addItem("Client (Connect)");
    layout->addWidget(m_modeCombo);

    // Host
    layout->addWidget(new QLabel("Host:", group));
    m_hostEdit = new QLineEdit(group);
    m_hostEdit->setPlaceholderText("192.168.x.x");
    m_hostEdit->setMaximumWidth(130);
    // Default: hidden in server mode
    layout->addWidget(m_hostEdit);

    // Port
    layout->addWidget(new QLabel("Port:", group));
    m_portSpin = new QSpinBox(group);
    m_portSpin->setRange(1024, 65535);
    m_portSpin->setValue(DEFAULT_PORT);
    m_portSpin->setMaximumWidth(80);
    layout->addWidget(m_portSpin);

    // Start/Stop
    m_startStopBtn = new QPushButton("Start", group);
    m_startStopBtn->setMinimumWidth(70);
    layout->addWidget(m_startStopBtn);

    // Status
    m_statusLabel = new QLabel("Not connected", group);
    m_statusLabel->setStyleSheet("color: gray; font-style: italic;");
    layout->addWidget(m_statusLabel);

    layout->addStretch();

    // Connections
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConnectionPanel::onModeChanged);
    connect(m_startStopBtn, &QPushButton::clicked,
            this, &ConnectionPanel::onStartStopClicked);

    // Default: server mode, hide host field
    onModeChanged(0);
}

bool ConnectionPanel::isServerMode() const {
    return m_modeCombo->currentIndex() == 0;
}

quint16 ConnectionPanel::port() const {
    return static_cast<quint16>(m_portSpin->value());
}

QString ConnectionPanel::host() const {
    return m_hostEdit->text();
}

void ConnectionPanel::setStatus(const QString& text) {
    m_statusLabel->setText(text);
}

void ConnectionPanel::setConnected(bool connected) {
    m_connected = connected;
    m_startStopBtn->setText(connected ? "Stop" : "Start");
    m_modeCombo->setEnabled(!connected);
    m_hostEdit->setEnabled(!connected && !isServerMode());
    m_portSpin->setEnabled(!connected);
}

void ConnectionPanel::onModeChanged(int index) {
    bool isServer = (index == 0);
    m_hostEdit->setVisible(!isServer);
    // Find the "Host:" label (it's 2 items before m_hostEdit in layout)
    // Simple approach: find the QLabel before m_hostEdit
    auto* group = qobject_cast<QGroupBox*>(parent()->findChild<QGroupBox*>());
    Q_UNUSED(group)
}

void ConnectionPanel::onStartStopClicked() {
    if (m_connected) {
        emit stopRequested();
    } else {
        if (isServerMode()) {
            emit startServerRequested(port());
        } else {
            emit connectRequested(host(), port());
        }
    }
}
