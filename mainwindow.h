#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QLabel>
#include <QList>
#include <QRegularExpression>
#include <QTimer>
#include <QPropertyAnimation> // Добавляем для плавной анимации

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override; // Для обработки изменения размера окна

private slots:
    void newConnection();
    void readClient();
    void clientDisconnected();
    void updateAnimationFrame(); // Обновление кадра анимации

private:
    void setupUI();
    void processMessage(QTcpSocket *clientSocket, const QString &message);
    void showStudentPhoto(const QString &surname);
    void startPhotoAnimation();
    void stopPhotoAnimation();
    void centerPhoto(); // Центрирование фото

    QTcpServer *tcpServer;
    QList<QTcpSocket*> clientSockets;

    // Элементы UI
    QWidget *mainContainer;
    QLabel *statusLabel;
    QLabel *photoLabel;

    // Анимация
    QTimer *animationTimer;
    QString currentPhotoSurname;
    bool isPhotoAnimating;
    int animationDirection;
    int animationOffset;
    int maxAnimationOffset;
};

#endif // MAINWINDOW_H
