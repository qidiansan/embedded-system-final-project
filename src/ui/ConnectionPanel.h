#ifndef CONNECTIONPANEL_H
#define CONNECTIONPANEL_H

#include "protocol/ProtocolDefs.h"
#include <QWidget>

class QLineEdit;
class QSpinBox;
class QComboBox;
class QPushButton;
class QLabel;

class ConnectionPanel : public QWidget {
    Q_OBJECT
public:
    explicit ConnectionPanel(QWidget* parent = nullptr);

    bool isServerMode() const;
    quint16 port() const;
    QString host() const;

signals:
    void startServerRequested(quint16 port);
    void connectRequested(const QString& host, quint16 port);
    void stopRequested();

public slots:
    void setStatus(const QString& text);
    void setConnected(bool connected);

private slots:
    void onModeChanged(int index);
    void onStartStopClicked();

private:
    QComboBox*   m_modeCombo;
    QLineEdit*   m_hostEdit;
    QSpinBox*    m_portSpin;
    QPushButton* m_startStopBtn;
    QLabel*      m_statusLabel;
    bool         m_connected = false;
};

#endif // CONNECTIONPANEL_H
