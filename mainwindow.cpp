#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QMessageBox>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setupUI();
    tcpSocket = new QTcpSocket(this);

    connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::readResponse);
    connect(tcpSocket, &QTcpSocket::errorOccurred, this, &MainWindow::displayError);
    connect(messageEdit, &QLineEdit::returnPressed, this, &MainWindow::handleReturnPressed);
}

MainWindow::~MainWindow()
{
    if(tcpSocket->state() == QAbstractSocket::ConnectedState)
        tcpSocket->disconnectFromHost();
}

void MainWindow::setupUI()
{
    setWindowTitle("Client");
    resize(600, 500);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *serverLayout = new QHBoxLayout();
    serverAddressEdit = new QLineEdit("172.20.10.6", this);
    connectButton = new QPushButton("Connect", this);

    serverLayout->addWidget(serverAddressEdit);
    serverLayout->addWidget(connectButton);

    chatDisplay = new QTextEdit(this);
    chatDisplay->setReadOnly(true);
    chatDisplay->setText("Not connected to server");

    messageEdit = new QLineEdit(this);
    messageEdit->setPlaceholderText("Enter message (Hello, Garson, I'm Surname!)");
    sendButton = new QPushButton("Send Message", this);
    sendButton->setEnabled(false);

    QHBoxLayout *messageLayout = new QHBoxLayout();
    messageLayout->addWidget(messageEdit);
    messageLayout->addWidget(sendButton);

    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumSize(300, 200);
    imageLabel->setFrameStyle(QFrame::Box);
    imageLabel->setText("Garden image will appear here");
    imageLabel->hide();

    statusLabel = new QLabel("Status: Not connected", this);

    mainLayout->addLayout(serverLayout);
    mainLayout->addWidget(chatDisplay);
    mainLayout->addLayout(messageLayout);
    mainLayout->addWidget(imageLabel);
    mainLayout->addWidget(statusLabel);

    setCentralWidget(centralWidget);

    connect(connectButton, &QPushButton::clicked, this, &MainWindow::connectToServer);
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendMessage);
}

void MainWindow::handleReturnPressed()
{
    if(!messageEdit->text().trimmed().isEmpty() && sendButton->isEnabled()) {
        sendMessage();
    }
}

void MainWindow::showGardenImage()
{
    QPixmap gardenImage(":/images/garden.jpg");

    if(!gardenImage.isNull()) {
        imageLabel->setPixmap(gardenImage.scaled(imageLabel->size(),
                                                 Qt::KeepAspectRatio,
                                                 Qt::SmoothTransformation));
        imageLabel->show();
    } else {
        imageLabel->setText("🌳🌲🌴 Garden Image Missing 🌴🌲🌳");
        imageLabel->show();
    }
}

void MainWindow::connectToServer()
{
    if(tcpSocket->state() == QAbstractSocket::ConnectedState)
        tcpSocket->disconnectFromHost();

    tcpSocket->connectToHost(serverAddressEdit->text(), 12345);

    if(tcpSocket->waitForConnected(3000)) {
        statusLabel->setText("Status: Connected to server");
        sendButton->setEnabled(true);
        appendMessage("System: Connected to server");
    } else {
        statusLabel->setText("Status: Connection error - " + tcpSocket->errorString());
    }
}

void MainWindow::sendMessage()
{
    QString message = messageEdit->text().trimmed();

    if(message.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter a message!");
        return;
    }

    // Сохраняем имя из сообщения
    QRegularExpression rx("Hello, Garson, I'm (\\w+)!");
    QRegularExpressionMatch match = rx.match(message);
    if (match.hasMatch()) {
        currentSurname = match.captured(1);
    }

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << message;

    tcpSocket->write(block);
    appendMessage("You: " + message);
    messageEdit->clear();
}

void MainWindow::readResponse()
{
    QDataStream in(tcpSocket);
    in.setVersion(QDataStream::Qt_6_0);

    QString response;
    in >> response;

    if(response.contains("Go To Sleep To the Garden!")) {
        appendMessage("Server: " + response);
        statusLabel->setText("Status: Correct message format");
        showStudentInGarden(currentSurname); // Используем сохраненное имя
    } else {
        appendMessage("Server: " + response);
        statusLabel->setText("Status: Incorrect message format");
        imageLabel->hide();
    }
}
void MainWindow::showStudentInGarden(const QString &surname) {
    // 1. Загружаем фон сада
    QPixmap gardenImage(":/images/garden.jpg");
    if (gardenImage.isNull()) {
        qDebug() << "Ошибка: garden.jpg не загружен!";
        gardenImage = QPixmap(300, 200);
        gardenImage.fill(Qt::green);
    }

    // 2. Загружаем фото студента
    QString studentPath = ":/images/" + surname + ".jpg";
    QPixmap studentImage(studentPath);

    if (studentImage.isNull()) {
        qDebug() << "Ошибка: фото студента не загружено! Путь:" << studentPath;
        studentImage = QPixmap(100, 100);
        studentImage.fill(Qt::blue);
        QPainter painter(&studentImage);
        painter.drawText(studentImage.rect(), Qt::AlignCenter, surname);
    }

    // 3. Создаем результирующее изображение
    QPixmap resultImage = gardenImage.scaled(300, 200, Qt::KeepAspectRatioByExpanding);
    QPainter painter(&resultImage);

    // 4. Размещаем фото студента
    int studentWidth = 200;
    int studentHeight = 200;
    int x = (resultImage.width() - studentWidth) / 2;
    int y = (resultImage.height() - studentHeight) / 2 + 40;

    QRect studentRect(x, y, studentWidth, studentHeight);
    painter.drawPixmap(studentRect, studentImage.scaled(studentWidth, studentHeight));

    // 5. Отображаем результат
    imageLabel->setPixmap(resultImage);
    imageLabel->show();

}

void MainWindow::appendMessage(const QString &message)
{
    chatDisplay->append(message);
}

void MainWindow::displayError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    QString error = "Network error: " + tcpSocket->errorString();
    statusLabel->setText("Status: " + error);
    appendMessage("System: " + error);
    sendButton->setEnabled(false);
    imageLabel->hide();
}
