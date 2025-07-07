#ifndef SERVERWINDOW_H
#define SERVERWINDOW_H

#include <QMainWindow>
#include "expression_server.h"

class QPlainTextEdit;
class QTableWidget;
class QLabel;

class ServerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ServerWindow(QWidget *parent = nullptr);
    ~ServerWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void updateHistoryView(const QVector<ExpressionServer::HistoryRecord>& records);
    void appendToLog(const QString& message, const QString& color);

    void sortHistoryByDate();
    void sortHistoryByType();
    void sortHistoryByLength();
    void sortHistoryByTime();

private:
    void setupUi();
    void setupMenu();
    void populateTable();

    QPlainTextEdit *logView;
    QTableWidget *historyTable;
    QLabel *statusLabel;
    ExpressionServer *serverLogic;
    QVector<ExpressionServer::HistoryRecord> currentHistory;
};

#endif // SERVERWINDOW_H