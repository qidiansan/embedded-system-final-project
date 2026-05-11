#ifndef LOGPANEL_H
#define LOGPANEL_H

#include <QWidget>

class QTextEdit;

class LogPanel : public QWidget {
    Q_OBJECT
public:
    explicit LogPanel(QWidget* parent = nullptr);

public slots:
    void appendLog(const QString& message);

private:
    QTextEdit* m_logView;
};

#endif // LOGPANEL_H
