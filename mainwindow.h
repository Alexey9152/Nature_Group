#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QTcpSocket>
#include <QFileDialog>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDataStream>
#include <QElapsedTimer>
#include <QDateTime>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QTableWidget>
#include <QHeaderView>
#include <QDialog>
#include <QDialogButtonBox>
#include "Date.h"

class HistoryDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    struct HistoryRecord {
        Date date;
        QString type;
        QString expression;
        QString rpn;
        QString result;
        qint64 processingTime;
    };

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void runProgram();
    void showAbout();
    void openFile();
    void clearOutput();
    void readServerResponse();
    void showHistory();
    void showServerHistory();
    void saveHistory();
    void sortHistoryByDate();
    void sortHistoryByType();
    void sortHistoryByLength();
    void sortHistoryByTime();

private:
    bool readExpressionAndOperands(QString &expression, std::map<std::string, double> &operands);
    void sendExpressionAndRPNToServer(const QString& expression, const QString& rpn);
    void sendCoefficientsToServer(const std::map<std::string, double>& operands);
    QString formatDouble(double value);
    void appendToOutput(const QString &text, const QString &color = "black");
    int getPrecedence(char op);
    bool isOperator(char c);
    bool convertToRPNClient(const QString& expression, QString& rpn);
    void addHistoryRecord(const QString &type, const QString &expression,
                          const QString &rpn, const QString &result, qint64 time = 0);

    QTextEdit *textEdit;
    QPushButton *runButton;
    QPushButton *aboutButton;
    QPushButton *openButton;
    QPushButton *clearButton;
    QLineEdit *ipEdit;
    QSpinBox *portSpin;
    QTcpSocket *tcpSocket;
    QString currentFile;

    QString currentExpression;
    std::map<std::string, double> currentOperands;
    QString currentClientRPN;
    QList<HistoryRecord> historyRecords;
    QElapsedTimer requestTimer;
    HistoryDialog *historyDialog;
};

class HistoryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HistoryDialog(const QList<MainWindow::HistoryRecord> &records,
                           QWidget *parent = nullptr);

private:
    QTableWidget *table;
    QList<MainWindow::HistoryRecord> records;
    void populateTable();
};

#endif // MAINWINDOW_H
