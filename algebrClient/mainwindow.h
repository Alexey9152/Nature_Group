#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QElapsedTimer>
#include <QDialog>
#include "Date.h"

enum class MessageType : quint16 {
    C2S_EXPRESSION_SUBMISSION,
    C2S_COEFFICIENTS_SUBMISSION,
    S2C_RPN_MATCH_REQUEST_COEFFS,
    S2C_RPN_MISMATCH_SEND_CORRECT,
    S2C_EXPRESSION_ERROR,
    S2C_FINAL_RESULT,
    S2C_CALCULATION_ERROR,
    S2C_PROTOCOL_ERROR
};

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

    struct BinaryLogEntry {
        char expression[80];
        unsigned char hour;
        unsigned char minute;
        unsigned char second;
        char rpn[80];
    };

    struct CoefficientEntry {
        char name[8];
        double value;
    };

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void runProgram();
    void showAbout();
    void openFile();
    void clearOutput();
    void readServerResponse();
    void onSocketStateChanged(QAbstractSocket::SocketState socketState);
    void onSocketError(QAbstractSocket::SocketError socketError);
    void showHistory();
    void saveHistory();
    void sortHistoryByDate();
    void sortHistoryByType();
    void sortHistoryByLength();
    void sortHistoryByTime();

private:
    void setupUi();
    void setupMenu();
    void sendInitialRequest();
    void sendCoefficientsToServer();
    bool readExpressionAndOperands();
    void appendToOutput(const QString &text, const QString &color = "black");
    void saveToBinaryLog(const QString &expr, const QString &rpn);
    void saveHistoryToBinary();
    int getPrecedence(char op);
    //bool isOperator(char c);
    bool convertToRPNClient(const QString &expression, QString &rpn);
    void addHistoryRecord(const QString &type,
                          const QString &expression,
                          const QString &rpn,
                          const QString &result,
                          qint64 time);
    bool isOpeningBracket(char c) const;
    bool isClosingBracket(char c) const;
    char getMatchingBracket(char c) const;
    bool isOperator(char c) const;

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
    QMap<QString, double> currentOperands;
    QString currentClientRPN;
    QList<HistoryRecord> historyRecords;
    QElapsedTimer *requestTimer;
    HistoryDialog *historyDialog = nullptr;
};

class HistoryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HistoryDialog(const QList<MainWindow::HistoryRecord> &records,
                           QWidget *parent = nullptr);
    ~HistoryDialog() override = default;
};

#endif // MAINWINDOW_H
