#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QHostAddress>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QPixmap>
#include <QTime>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentPhotoSurname(""), isPhotoAnimating(false), animationDirection(1)
{
    setupUI();

    // Инициализация таймера анимации
    animationTimer = new QTimer(this);
    connect(animationTimer, &QTimer::timeout, this, &MainWindow::updateAnimation);

    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, 12345)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось запустить сервер: " + tcpServer->errorString());
        close();
        return;
    }

    connect(tcpServer, &QTcpServer::newConnection, this, &MainWindow::newConnection);
    statusLabel->setText("Сервер запущен на порту 12345\nОжидание подключений...");
}

MainWindow::~MainWindow()
{
    stopPhotoAnimation();
    qDeleteAll(clientSockets);
    delete animationTimer;
}

void MainWindow::setupUI()
{
    setWindowTitle("Сервер Задний конец");
    resize(600, 500);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Статус сервера
    statusLabel = new QLabel(this);
    statusLabel->setAlignment(Qt::AlignCenter);

    // Область для фото
    photoLabel = new QLabel(this);
    photoLabel->setAlignment(Qt::AlignCenter);
    photoLabel->setMinimumSize(300, 300);
    photoLabel->setFrameStyle(QFrame::Box);
    photoLabel->setText("Фото появится здесь\nпосле получения сообщения");

    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(photoLabel, 1);
    setCentralWidget(centralWidget);
}

void MainWindow::newConnection()
{
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &MainWindow::readClient);
    connect(clientSocket, &QTcpSocket::disconnected, this, &MainWindow::clientDisconnected);

    clientSockets.append(clientSocket);
    statusLabel->setText(QString("Подключен новый клиент\nВсего клиентов: %1").arg(clientSockets.size()));
}

void MainWindow::readClient()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    QDataStream in(clientSocket);
    in.setVersion(QDataStream::Qt_6_0);

    QString message;
    in >> message;

    // Останавливаем текущую анимацию перед обработкой нового сообщения
    stopPhotoAnimation();

    processMessage(clientSocket, message);
}

void MainWindow::processMessage(QTcpSocket *clientSocket, const QString &message)
{
    // Используем QRegularExpression
    QRegularExpression rx("Hello, Garson, I'm (\\w+)!");
    QRegularExpressionMatch match = rx.match(message);

    if (match.hasMatch()) {
        QString surname = match.captured(1);
        currentPhotoSurname = surname; // Сохраняем фамилию для анимации

        // Отправка ответа клиенту
        QString response = "I'm not Garson, I'm Server! Go To Sleep To the Garden!";
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_6_0);
        out << response;
        clientSocket->write(block);

        // Отображение информации о студенте и запуск анимации
        showStudentPhoto(surname);
        startPhotoAnimation();

        statusLabel->setText("Отправлен ответ клиенту:\n" + response);
    } else {
        // Ошибка формата
        QString error = "Error: Invalid message format! Expected: 'Hello, Garson, I'm [Surname]!'";
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_6_0);
        out << error;
        clientSocket->write(block);

        statusLabel->setText("Ошибка в сообщении:\n" + message);
    }
}

void MainWindow::showStudentPhoto(const QString &surname)
{
    // Попытка загрузить реальное фото
    QString imagePath = ":/" + surname + ".jpg";
    QPixmap studentPhoto(imagePath);

    if (!studentPhoto.isNull()) {
        QPixmap scaledPhoto = studentPhoto.scaled(photoLabel->size().width() - 20,
                                                  photoLabel->size().height() - 20,
                                                  Qt::KeepAspectRatio,
                                                  Qt::SmoothTransformation);
        photoLabel->setPixmap(scaledPhoto);
    } else {
        // Текстовая заглушка
        photoLabel->setText("🖼️ Фото студента: " + surname + "\n"
                                                             "🔁 Анимация прыжков активна\n"
                                                             "✅ Сообщение обработано успешно");
    }
}

void MainWindow::startPhotoAnimation()
{
    // Останавливаем предыдущую анимацию
    stopPhotoAnimation();

    // Запускаем новую анимацию
    isPhotoAnimating = true;
    animationDirection = 1; // Начинаем движение вниз
    animationTimer->start(50); // Обновление каждые 50 мс (20 FPS)
}

void MainWindow::stopPhotoAnimation()
{
    if (isPhotoAnimating) {
        animationTimer->stop();
        isPhotoAnimating = false;

        // Возвращаем фото в исходное положение
        photoLabel->move(photoLabel->x(), photoLabel->y() - 10);
    }
}

void MainWindow::updateAnimation()
{
    if (!isPhotoAnimating) return;

    // Текущая позиция фото
    int currentY = photoLabel->y();

    // Максимальное смещение (10 пикселей)
    const int maxOffset = 10;

    // Изменяем позицию
    if (animationDirection == 1) {
        // Движение вниз
        if (currentY < maxOffset) {
            photoLabel->move(photoLabel->x(), currentY + 1);
        } else {
            animationDirection = -1; // Меняем направление
        }
    } else {
        // Движение вверх
        if (currentY > -maxOffset) {
            photoLabel->move(photoLabel->x(), currentY - 1);
        } else {
            animationDirection = 1; // Меняем направление
        }
    }
}

void MainWindow::clientDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket) {
        clientSockets.removeOne(clientSocket);
        clientSocket->deleteLater();
        statusLabel->setText(QString("Клиент отключен\nОсталось клиентов: %1").arg(clientSockets.size()));
    }
}
