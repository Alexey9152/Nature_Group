#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QMessageBox>
#include <QPixmap>

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
        showGardenImage();
    } else {
        appendMessage("Server: " + response);
        statusLabel->setText("Status: Incorrect message format");
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
}
