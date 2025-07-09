#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QHostAddress>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QPixmap>
#include <QResizeEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentPhotoSurname(""),
    isPhotoAnimating(false), animationDirection(1),
    animationOffset(0), maxAnimationOffset(20)
{
    setupUI();

    // Инициализация таймера анимации
    animationTimer = new QTimer(this);
    connect(animationTimer, &QTimer::timeout, this, &MainWindow::updateAnimationFrame);

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
    setWindowTitle("Server, LipovskiyMatvei, Липовский М.В. ");
    resize(800, 600); // Увеличиваем размер окна

    // Главный контейнер
    mainContainer = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainContainer);
    mainLayout->setSpacing(20);

    // Статус сервера (верхняя часть)
    statusLabel = new QLabel(this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    statusLabel->setMinimumHeight(40);

    // Контейнер для фото с фиксированной высотой
    QWidget *photoContainer = new QWidget(this);
    photoContainer->setMinimumHeight(400);
    photoContainer->setStyleSheet("background-color: #f0f0f0; border: 1px solid #ccc;");

    QVBoxLayout *containerLayout = new QVBoxLayout(photoContainer);
    containerLayout->setContentsMargins(10, 10, 10, 10);

    // Область для фото
    photoLabel = new QLabel(photoContainer);
    photoLabel->setAlignment(Qt::AlignCenter);
    photoLabel->setStyleSheet("background-color: white;");

    containerLayout->addWidget(photoLabel);

    // Добавляем элементы в главный макет
    mainLayout->addWidget(statusLabel, 0);
    mainLayout->addWidget(photoContainer, 1);

    setCentralWidget(mainContainer);

    // Инициализируем фото
    photoLabel->setText("Фото появится здесь\nпосле получения сообщения");
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (!currentPhotoSurname.isEmpty()) {
        showStudentPhoto(currentPhotoSurname);
    }
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
    animationOffset = 0;

    processMessage(clientSocket, message);
}

void MainWindow::processMessage(QTcpSocket *clientSocket, const QString &message)
{
    // Используем QRegularExpression
    QRegularExpression rx("Hello, Garson, I'm (\\w+)!");
    QRegularExpressionMatch match = rx.match(message);

    if (match.hasMatch()) {
        QString surname = match.captured(1);
        currentPhotoSurname = surname;

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
        // Рассчитываем размер с учетом места для анимации
        int maxWidth = photoLabel->parentWidget()->width() - 50;
        int maxHeight = photoLabel->parentWidget()->height() - 50 - maxAnimationOffset;

        QPixmap scaledPhoto = studentPhoto.scaled(
            maxWidth,
            maxHeight,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
            );

        photoLabel->setPixmap(scaledPhoto);
        photoLabel->resize(scaledPhoto.size());
        centerPhoto();
    } else {
        // Текстовая заглушка
        photoLabel->setText("🖼️ Фото студента: " + surname + "\n"
                                                             "🔁 Анимация прыжков активна\n"
                                                             "✅ Сообщение обработано успешно");
        photoLabel->resize(photoLabel->sizeHint());
        centerPhoto();
    }
}

void MainWindow::centerPhoto()
{
    QWidget *parent = photoLabel->parentWidget();
    int x = (parent->width() - photoLabel->width()) / 2;
    int y = (parent->height() - photoLabel->height() - maxAnimationOffset) / 2;
    photoLabel->move(x, y);
}

void MainWindow::startPhotoAnimation()
{
    if (!isPhotoAnimating) {
        isPhotoAnimating = true;
        animationTimer->start(50); // 20 FPS
    }
}

void MainWindow::stopPhotoAnimation()
{
    if (isPhotoAnimating) {
        animationTimer->stop();
        isPhotoAnimating = false;
    }
}

void MainWindow::updateAnimationFrame()
{
    if (!isPhotoAnimating) return;

    // Обновляем смещение
    animationOffset += animationDirection * 2;

    // Меняем направление при достижении границ
    if (animationOffset >= maxAnimationOffset || animationOffset <= -maxAnimationOffset) {
        animationDirection *= -1;
    }

    // Применяем смещение к позиции фото
    QPoint pos = photoLabel->pos();
    photoLabel->move(pos.x(), pos.y() - animationDirection * 1);
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
