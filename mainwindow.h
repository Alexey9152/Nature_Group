#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QFileDialog>
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
    void showGardenImage();

    QTcpSocket *tcpSocket;
    QLabel *statusLabel;
    QTextEdit *chatDisplay;
    QLineEdit *messageEdit;
    QLineEdit *serverAddressEdit;
    QPushButton *connectButton;
    QPushButton *sendButton;
    QPushButton *selectFileButton;
    QPushButton *sendFileButton;
    QString currentFilePath;
    QLabel *imageLabel;
};

#endif // MAINWINDOW_H
