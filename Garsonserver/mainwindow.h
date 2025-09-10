#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QLabel>
#include <QList>
#include <QRegularExpression>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QRandomGenerator>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void newConnection();
    void readClient();
    void clientDisconnected();
    void animatePhotoMovement();

private:
    void setupUI();
    void processMessage(QTcpSocket *clientSocket, const QString &message);
    void showStudentPhoto(const QString &surname);
    void startPhotoAnimation();
    void stopPhotoAnimation();

    QTcpServer *tcpServer;
    QList<QTcpSocket*> clientSockets;

    QLabel *statusLabel;
    QLabel *photoLabel;
    QPropertyAnimation *photoAnimation;
    QSequentialAnimationGroup *animationGroup;
    QString currentDisplayedSurname;
};

#endif // MAINWINDOW_H
