#ifndef TRANSFERPANEL_H
#define TRANSFERPANEL_H

#include <QWidget>

class QTableView;
class QStandardItemModel;
class QPushButton;
class QLabel;

class TransferPanel : public QWidget {
    Q_OBJECT
public:
    explicit TransferPanel(QWidget* parent = nullptr);

    int addTransfer(const QString& fileName, bool isSend, qint64 size);
    void updateProgress(int row, int percent, const QString& speed = QString());
    void setProgressBar(int row, int percent);
    void setStatus(int row, const QString& status);
    void removeRow(int row);
    void clearCompleted();

signals:
    void sendFileRequested(const QString& localPath);
    void sendDirRequested(const QString& localPath);
    void cancelRequested(int row);

private slots:
    void onSendFile();
    void onSendDir();
    void onCancel();
    void onClearDone();

private:
    QTableView*         m_table;
    QStandardItemModel*  m_model;
    QPushButton*        m_sendFileBtn;
    QPushButton*        m_sendDirBtn;
    QPushButton*        m_cancelBtn;
    QPushButton*        m_clearBtn;
};

#endif // TRANSFERPANEL_H
