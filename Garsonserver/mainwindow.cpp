#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QHostAddress>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QPixmap>
#include <QDebug>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();

    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, 12345)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось запустить сервер: " + tcpServer->errorString());
        close();
        return;
    }

    connect(tcpServer, &QTcpServer::newConnection, this, &MainWindow::newConnection);
    statusLabel->setText("Сервер запущен на порту 12345\nОжидание подключений...");

    photoAnimation = new QPropertyAnimation(photoLabel, "pos", this);
    animationGroup = new QSequentialAnimationGroup(this);
    animationGroup->setLoopCount(-1);
    animationGroup->addAnimation(photoAnimation);
    connect(photoAnimation, &QPropertyAnimation::finished, this, &MainWindow::animatePhotoMovement);
}

MainWindow::~MainWindow()
{
    qDeleteAll(clientSockets);
    stopPhotoAnimation();
}

void MainWindow::setupUI()
{
    setWindowTitle("server");
    resize(600, 500);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    statusLabel = new QLabel(this);
    statusLabel->setAlignment(Qt::AlignCenter);

    photoLabel = new QLabel(this);
    photoLabel->setAlignment(Qt::AlignCenter);
    photoLabel->setFixedSize(170, 170);
    photoLabel->setFrameStyle(QFrame::Box);
    photoLabel->setText("Фото появится здесь\nпосле получения сообщения");

    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(photoLabel, 1, Qt::AlignCenter);
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

    processMessage(clientSocket, message);
}

void MainWindow::processMessage(QTcpSocket *clientSocket, const QString &message)
{
    QRegularExpression fullRx("^Hello, Garson, I'm (\\w+)!$");
    QRegularExpressionMatch fullMatch = fullRx.match(message);
    QString response;

    if (fullMatch.hasMatch()) {
        QString surname = fullMatch.captured(1);
        QString imagePath = ":/images/" + surname + ".jpg";
        QPixmap studentPhoto(imagePath);

        if (!studentPhoto.isNull()) {
            if (currentDisplayedSurname != surname || photoLabel->pixmap().isNull()) {
                currentDisplayedSurname = surname;
                showStudentPhoto(surname);
                startPhotoAnimation();
            }
            response = "I'm not Garson, I'm Server! Go To Sleep To the Garden!";
            statusLabel->setText("Отправлен ответ клиенту:\n" + response + "\nОтображение фото " + surname + " с анимацией.");
        } else {
            response = "Student " + surname + " is not in group 7!";
            statusLabel->setText("Отправлен ответ клиенту:\n" + response + "\nФото не найдено.");
        }
    } else {
        response = "Error: Invalid message format! Details:\n";
        bool errorFoundAndReported = false; // Флаг для остановки после первой найденной ошибки

        if (!message.startsWith("Hello")) {
            response += "- Сообщение должно начинаться с 'Hello'. ";
            if (message.length() >= 5 && message.left(5).toLower() == "hello") {
                response += "Проверьте регистр 'Hello'. ";
            }
            errorFoundAndReported = true;
        } else if (message.length() >= 5 && message.left(5) != "Hello") {
            response += "- 'Hello' имеет неправильный регистр. ";
            errorFoundAndReported = true;
        }

        if (!errorFoundAndReported) {
            int garsonIdx = message.indexOf("Garson");
            if (garsonIdx == -1) {
                response += "- Отсутствует ключевое слово 'Garson'. ";
                errorFoundAndReported = true;
            } else {
                if (garsonIdx < 2 || message.at(garsonIdx - 1) != ' ' || message.at(garsonIdx - 2) != ',') {
                    response += "- Неправильный пробел или отсутствует ', ' перед 'Garson'. ";
                    errorFoundAndReported = true;
                } else {
                    int imIdx = message.indexOf("I'm", garsonIdx + 6);
                    if (imIdx != -1) {
                        if (imIdx < garsonIdx + 6 || message.at(imIdx - 1) != ' ' || message.at(imIdx - 2) != ',') {
                            response += "- Неправильный пробел или отсутствует ', ' перед 'I'm'. ";
                            errorFoundAndReported = true;
                        }
                    }
                }
            }
        }

        if (!errorFoundAndReported) {
            int imKeywordIdx = message.indexOf("I'm");
            if (imKeywordIdx == -1) {
                response += "- Отсутствует ключевое слово 'I'm'. ";
                errorFoundAndReported = true;
            } else {
                if (imKeywordIdx > 0 && message.at(imKeywordIdx - 1) != ' ') {
                    response += "- Отсутствует пробел перед 'I'm'. ";
                    errorFoundAndReported = true;
                }
                if (imKeywordIdx + 2 < message.length() && message.at(imKeywordIdx + 1) != '\'') {
                    response += "- Отсутствует апостроф в 'I'm'. ";
                    errorFoundAndReported = true;
                }
            }
        }

        if (!errorFoundAndReported) {
            if (!message.endsWith("!")) {
                response += "- Сообщение должно заканчиваться восклицательным знаком '!'. ";
                errorFoundAndReported = true;
            }
        }

        if (!errorFoundAndReported) {
            response += "- Общая ошибка формата. Проверьте лишние символы, неправильный порядок или неизвестные элементы. ";
        }

        response += "\nОжидаемый формат: 'Hello, Garson, I'm [Фамилия]!'";
        statusLabel->setText("Error in message:\n" + message + "\nPrevious animation continues.");
    }

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << response;
    clientSocket->write(block);
}

void MainWindow::showStudentPhoto(const QString &surname)
{
    QString imagePath = ":/images/" + surname + ".jpg";
    QPixmap studentPhoto(imagePath);

    if (!studentPhoto.isNull()) {
        photoLabel->setPixmap(studentPhoto.scaled(photoLabel->size(),
                                                  Qt::KeepAspectRatio,
                                                  Qt::SmoothTransformation));
    } else {
        photoLabel->setText("🖼️ Фото для " + surname + " не найдено!\n"
                                                       "✅ Сообщение обработано успешно.\n"
                                                       "Запуск анимации (только текст, если нет изображения).");
    }
}

void MainWindow::startPhotoAnimation()
{
    if (photoAnimation->state() == QAbstractAnimation::Running) {
        photoAnimation->stop();
    }
    if (animationGroup->state() == QAbstractAnimation::Running) {
        animationGroup->stop();
    }

    QRect parentRect = centralWidget()->geometry();
    int maxX = parentRect.width() - photoLabel->width();
    int maxY = parentRect.height() - photoLabel->height();

    maxX = qMax(0, maxX);
    maxY = qMax(0, maxY);

    QPoint startPos = photoLabel->pos();
    QPoint endPos(QRandomGenerator::global()->bounded(maxX),
                  QRandomGenerator::global()->bounded(maxY));

    photoAnimation->setStartValue(startPos);
    photoAnimation->setEndValue(endPos);
    photoAnimation->setDuration(2000 + QRandomGenerator::global()->bounded(1000));

    animationGroup->start();
}

void MainWindow::animatePhotoMovement()
{
    if (!currentDisplayedSurname.isEmpty() && !photoLabel->pixmap().isNull()) {
        startPhotoAnimation();
    } else {
        stopPhotoAnimation();
    }
}

void MainWindow::stopPhotoAnimation()
{
    if (photoAnimation->state() == QAbstractAnimation::Running) {
        photoAnimation->stop();
    }
    if (animationGroup->state() == QAbstractAnimation::Running) {
        animationGroup->stop();
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
    if (clientSockets.isEmpty()) {
        stopPhotoAnimation();
        currentDisplayedSurname.clear();
        photoLabel->setText("Фото появится здесь\nпосле получения сообщения");
        photoLabel->clear();
    }
}
