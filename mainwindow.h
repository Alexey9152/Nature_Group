#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QTcpSocket>
#include "Date.h"
#include <QMainWindow>
#include <QDialog>         // for HistoryDialog inheritance
#include <QTextEdit>       // for textEdit
#include <QSpinBox>        // for portSpin
#include <QLineEdit>       // for ipEdit
#include <QPushButton>     // for runButton, aboutButton, etc.
#include <QLabel>          // if you use any QLabel
#include <QVBoxLayout>     // for layouts
#include <QHBoxLayout>
#include <QTcpSocket>      // for tcpSocket (ensure QT += network)


class QTextEdit;
class QPushButton;
class QLineEdit;
class QSpinBox;
class QTableWidget;
class QDialog;
class QElapsedTimer;

enum class MessageType : quint16 {
    C2S_EXPRESSION_SUBMISSION,   C2S_COEFFICIENTS_SUBMISSION,
    S2C_RPN_MATCH_REQUEST_COEFFS, S2C_RPN_MISMATCH_SEND_CORRECT,
    S2C_EXPRESSION_ERROR,         S2C_FINAL_RESULT,
    S2C_CALCULATION_ERROR,        S2C_PROTOCOL_ERROR
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
    int getPrecedence(char op);
    bool isOperator(char c);
    bool convertToRPNClient(const QString& expression, QString& rpn);
    void addHistoryRecord(const QString &type, const QString &expression,
                          const QString &rpn, const QString &result, qint64 time);

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
    explicit HistoryDialog(const QList<MainWindow::HistoryRecord> &records, QWidget *parent = nullptr);
};

#endif // MAINWINDOW_H
