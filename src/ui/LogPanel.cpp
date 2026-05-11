#include "LogPanel.h"
#include <QTextEdit>
#include <QVBoxLayout>
#include <QTime>
#include <QLabel>
#include <QScrollBar>

LogPanel::LogPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* title = new QLabel("Log", this);
    title->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(title);

    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setFont(QFont("Consolas", 10));
    m_logView->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4;");
    layout->addWidget(m_logView);
}

void LogPanel::appendLog(const QString& message) {
    QString timestamp = QTime::currentTime().toString("HH:mm:ss");
    m_logView->append(QString("[%1] %2").arg(timestamp, message));
    // Auto-scroll to bottom
    m_logView->verticalScrollBar()->setValue(
        m_logView->verticalScrollBar()->maximum());
}
