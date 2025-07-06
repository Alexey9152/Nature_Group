#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QLabel>
#include <QList>
#include <QRegularExpression>
#include <QTimer> // Добавляем таймер
#include <QPropertyAnimation> // Добавляем для анимации

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
    void updateAnimation(); // Слот для обновления анимации

private:
    void setupUI();
    void processMessage(QTcpSocket *clientSocket, const QString &message);
    void showStudentPhoto(const QString &surname);
    void startPhotoAnimation(); // Запуск анимации
    void stopPhotoAnimation(); // Остановка анимации

    QTcpServer *tcpServer;
    QList<QTcpSocket*> clientSockets;

    QLabel *statusLabel;
    QLabel *photoLabel;

    // Анимационные элементы
    QTimer *animationTimer;
    QPropertyAnimation *photoAnimation;
    QString currentPhotoSurname; // Фамилия текущего студента
    bool isPhotoAnimating; // Флаг активности анимации
    int animationDirection; // Направление движения
};

#endif // MAINWINDOW_H
