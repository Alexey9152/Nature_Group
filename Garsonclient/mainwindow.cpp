#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QMessageBox>
#include <QPixmap>
#include <QDebug>

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
    messageEdit->setPlaceholderText("Введите сообщение(например, Hello, Garson, I'm Surname!)");
    sendButton = new QPushButton("Отправить", this);
    sendButton->setEnabled(false);

    QHBoxLayout *messageLayout = new QHBoxLayout();
    messageLayout->addWidget(messageEdit);
    messageLayout->addWidget(sendButton);

    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumSize(300, 200);
    imageLabel->setFrameStyle(QFrame::Box);
    imageLabel->setText("Место для изображения");
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

void MainWindow::showSurnameImage(const QString &surname)
{
    QString imagePath = ":/images/" + surname + ".jpg";
    QPixmap surnameSleepingImage(imagePath);

    if(!surnameSleepingImage.isNull()) {
        imageLabel->setPixmap(surnameSleepingImage.scaled(imageLabel->size(),
                                                          Qt::KeepAspectRatio,
                                                          Qt::SmoothTransformation));
        imageLabel->show();
        statusLabel->setText("Status: Displaying " + surname + "'s sleeping photo!");
    } else {
        imageLabel->hide(); // <-- Изменено: скрываем место для фотографии
        statusLabel->setText("Status: Student " + surname + " is not in group 7! 😴"); // <-- Сообщение теперь в статусной строке
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
        QMessageBox::critical(this, "Connection Error", "Could not connect to the server. Please check the IP address and ensure the server is running.");
    }
}

void MainWindow::sendMessage()
{
    QString message = messageEdit->text().trimmed();

    if(message.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Пожалуйста, введите сообщение!");
        return;
    }

    QRegularExpression rx("Hello, Garson, I'm (\\w+)!");
    QRegularExpressionMatch match = rx.match(message);
    if (match.hasMatch()) {
        lastSentSurname = match.captured(1);
    } else {
        lastSentSurname.clear();
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

    appendMessage("Server: " + response);

    if(response.contains("Go To Sleep To the Garden!")) {
        statusLabel->setText("Status: Correct message format acknowledged!");
        if (!lastSentSurname.isEmpty()) {
            showSurnameImage(lastSentSurname);
        } else {
            imageLabel->setText("Server confirmed success, but no surname was extracted from your last message. Please ensure your message format is 'Hello, Garson, I'm Surname!'.");
            imageLabel->show();
        }
    } else if (response.startsWith("Error: Invalid message format!")) {
        statusLabel->setText("Status: Server reports an error in message format. Please correct.");
        QMessageBox::warning(this, "Message Error", "Server reported an error:\n" + response + "\nPlease correct your message and try again.");
        imageLabel->hide();
    } else {
        statusLabel->setText("Status: Unexpected server response.");
        imageLabel->hide();
    }
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
    QMessageBox::critical(this, "Socket Error", "A network error occurred: " + tcpSocket->errorString());
}
