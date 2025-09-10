#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QRegularExpression>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void connectToServer();
    void sendMessage();
    void readResponse();
    void displayError(QAbstractSocket::SocketError socketError);
    void handleReturnPressed();

private:
    void setupUI();
    void appendMessage(const QString &message);
    void showSurnameImage(const QString &surname);

    QTcpSocket *tcpSocket;
    QLabel *statusLabel;
    QTextEdit *chatDisplay;
    QLineEdit *messageEdit;
    QLineEdit *serverAddressEdit;
    QPushButton *connectButton;
    QPushButton *sendButton;
    QLabel *imageLabel;
    QString lastSentSurname;


    QString currentSurname;
    void showStudentInGarden(const QString &surname);
};

#endif // MAINWINDOW_H
