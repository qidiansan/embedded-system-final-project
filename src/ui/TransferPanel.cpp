#include "TransferPanel.h"
#include <QTableView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QProgressBar>

static QString formatSize(qint64 bytes) {
    if (bytes < 1024) return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024LL * 1024 * 1024) return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}

TransferPanel::TransferPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto* group = new QGroupBox("Transfers", this);
    auto* layout = new QVBoxLayout(group);
    outer->addWidget(group);

    // Toolbar
    auto* toolbar = new QHBoxLayout();
    m_sendFileBtn = new QPushButton("Send File", group);
    m_sendDirBtn  = new QPushButton("Send Directory", group);
    m_cancelBtn   = new QPushButton("Cancel", group);
    m_clearBtn    = new QPushButton("Clear Done", group);
    toolbar->addWidget(m_sendFileBtn);
    toolbar->addWidget(m_sendDirBtn);
    toolbar->addWidget(m_cancelBtn);
    toolbar->addWidget(m_clearBtn);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    // Table
    m_model = new QStandardItemModel(0, 5, this);
    m_model->setHorizontalHeaderLabels({"Name", "Direction", "Size", "Progress", "Status"});

    m_table = new QTableView(group);
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);
    layout->addWidget(m_table);

    // Connections
    connect(m_sendFileBtn, &QPushButton::clicked, this, &TransferPanel::onSendFile);
    connect(m_sendDirBtn, &QPushButton::clicked, this, &TransferPanel::onSendDir);
    connect(m_cancelBtn, &QPushButton::clicked, this, &TransferPanel::onCancel);
    connect(m_clearBtn, &QPushButton::clicked, this, &TransferPanel::onClearDone);
}

int TransferPanel::addTransfer(const QString& fileName, bool isSend, qint64 size) {
    int row = m_model->rowCount();
    m_model->insertRow(row);
    m_model->setItem(row, 0, new QStandardItem(fileName));
    m_model->setItem(row, 1, new QStandardItem(isSend ? "Send" : "Receive"));
    m_model->setItem(row, 2, new QStandardItem(formatSize(size)));
    // Progress bar
    auto* progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setTextVisible(true);
    m_table->setIndexWidget(m_model->index(row, 3), progressBar);
    m_model->setItem(row, 4, new QStandardItem("Queued"));
    return row;
}

void TransferPanel::updateProgress(int row, int percent, const QString& speed) {
    if (row < 0 || row >= m_model->rowCount()) return;
    auto* w = m_table->indexWidget(m_model->index(row, 3));
    auto* bar = qobject_cast<QProgressBar*>(w);
    if (bar) bar->setValue(percent);
    QString status = speed.isEmpty()
        ? QString("In Progress (%1%)").arg(percent)
        : QString("In Progress (%1% - %2)").arg(percent).arg(speed);
    m_model->item(row, 4)->setText(status);
}

void TransferPanel::setStatus(int row, const QString& status) {
    if (row < 0 || row >= m_model->rowCount()) return;
    m_model->item(row, 4)->setText(status);
}

void TransferPanel::removeRow(int row) {
    if (row < 0 || row >= m_model->rowCount()) return;
    m_model->removeRow(row);
}

void TransferPanel::clearCompleted() {
    for (int i = m_model->rowCount() - 1; i >= 0; --i) {
        QString status = m_model->item(i, 4)->text();
        if (status == "Complete" || status.startsWith("Error")) {
            m_model->removeRow(i);
        }
    }
}

void TransferPanel::onSendFile() {
    QString path = QFileDialog::getOpenFileName(this, "Select File to Send");
    if (!path.isEmpty())
        emit sendFileRequested(path);
}

void TransferPanel::onSendDir() {
    QString path = QFileDialog::getExistingDirectory(this, "Select Directory to Send");
    if (!path.isEmpty())
        emit sendDirRequested(path);
}

void TransferPanel::onCancel() {
    QModelIndex idx = m_table->currentIndex();
    if (idx.isValid())
        emit cancelRequested(idx.row());
}

void TransferPanel::onClearDone() {
    clearCompleted();
}
