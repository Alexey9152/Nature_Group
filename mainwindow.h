#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QTextEdit>

#include <QCache>

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

    QString currentSurname;
    void showStudentInGarden(const QString &surname);
};

#endif // MAINWINDOW_H
