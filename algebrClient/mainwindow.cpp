#include "mainwindow.h"
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QTime>
#include <stack>
#include <cstring>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    textEdit(nullptr),
    runButton(nullptr),
    aboutButton(nullptr),
    openButton(nullptr),
    clearButton(nullptr),
    ipEdit(nullptr),
    portSpin(nullptr),
    tcpSocket(nullptr),
    requestTimer(new QElapsedTimer()),
    historyDialog(nullptr)
{
    setupUi();
    setupMenu();

    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::readServerResponse);
    connect(tcpSocket, &QTcpSocket::stateChanged, this, &MainWindow::onSocketStateChanged);
    connect(tcpSocket, &QAbstractSocket::errorOccurred, this, &MainWindow::onSocketError);

    appendToOutput("Готов к работе.", "cyan");
    appendToOutput("Выберите файл и нажмите 'Запустить'", "cyan");
}

MainWindow::~MainWindow()
{
    delete requestTimer;
    if (historyDialog) delete historyDialog;
}

void MainWindow::setupUi()
{
    setWindowTitle("Клиент калькулятора выражений");
    resize(800, 600);

    textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);

    runButton = new QPushButton("Запустить", this);
    aboutButton = new QPushButton("О программе", this);
    openButton = new QPushButton("Открыть файл", this);
    clearButton = new QPushButton("Очистить", this);

    ipEdit = new QLineEdit("localhost", this);
    portSpin = new QSpinBox(this);
    portSpin->setRange(1, 65535);
    portSpin->setValue(12345);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *serverLayout = new QHBoxLayout();
    serverLayout->addWidget(new QLabel("Сервер:", this));
    serverLayout->addWidget(ipEdit);
    serverLayout->addWidget(new QLabel("Порт:", this));
    serverLayout->addWidget(portSpin);

    mainLayout->addLayout(serverLayout);
    mainLayout->addWidget(textEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(openButton);
    buttonLayout->addWidget(runButton);
    buttonLayout->addWidget(aboutButton);
    buttonLayout->addWidget(clearButton);

    mainLayout->addLayout(buttonLayout);
    setCentralWidget(centralWidget);

    connect(runButton, &QPushButton::clicked, this, &MainWindow::runProgram);
    connect(aboutButton, &QPushButton::clicked, this, &MainWindow::showAbout);
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openFile);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearOutput);
}

void MainWindow::setupMenu()
{
    QMenu *historyMenu = menuBar()->addMenu("История");
    historyMenu->addAction("Показать историю", this, &MainWindow::showHistory);
    historyMenu->addAction("Сохранить историю", this, &MainWindow::saveHistory);

    QMenu *sortMenu = menuBar()->addMenu("Сортировка");
    sortMenu->addAction("По дате", this, &MainWindow::sortHistoryByDate);
    sortMenu->addAction("По типу запроса", this, &MainWindow::sortHistoryByType);
    sortMenu->addAction("По длине выражения", this, &MainWindow::sortHistoryByLength);
    sortMenu->addAction("По времени обработки", this, &MainWindow::sortHistoryByTime);
}

void MainWindow::runProgram()
{
    if (currentFile.isEmpty()) {
        appendToOutput("Ошибка: Файл не выбран", "red");
        return;
    }

    textEdit->clear();
    appendToOutput("Обработка файла: " + currentFile, "white");

    if (!readExpressionAndOperands()) {
        appendToOutput("Ошибка чтения файла", "red");
        return;
    }

    if (!convertToRPNClient(currentExpression, currentClientRPN)) {
        appendToOutput("Ошибка преобразования в ОПЗ", "red");
        return;
    }

    appendToOutput("Выражение: " + currentExpression, "blue");
    appendToOutput("ОПЗ клиента: " + currentClientRPN, "blue");

    for (auto it = currentOperands.constBegin(); it != currentOperands.constEnd(); ++it) {
        appendToOutput(QString("Коэффициент %1 = %2").arg(it.key()).arg(it.value()), "blue");
    }

    requestTimer->start();
    appendToOutput("Подключение к серверу...", "gray");
    tcpSocket->connectToHost(ipEdit->text(), static_cast<quint16>(portSpin->value()));
}

void MainWindow::sendInitialRequest()
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);

    out << quint16(0);
    out << static_cast<quint16>(MessageType::C2S_EXPRESSION_SUBMISSION);
    out << currentExpression;
    out << currentClientRPN;
    out.device()->seek(0);
    out << quint16(block.size() - sizeof(quint16));

    tcpSocket->write(block);
    saveToBinaryLog(currentExpression, currentClientRPN);
    appendToOutput("Запрос отправлен на сервер", "blue");
}

void MainWindow::sendCoefficientsToServer()
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << quint16(0);
    out << static_cast<quint16>(MessageType::C2S_COEFFICIENTS_SUBMISSION);
    out << quint32(currentOperands.size());
    for (auto it = currentOperands.constBegin(); it != currentOperands.constEnd(); ++it) {
        out << it.key() << it.value();
    }
    out.device()->seek(0);
    out << quint16(block.size() - sizeof(quint16));

    tcpSocket->write(block);
    appendToOutput("Коэффициенты отправлены", "blue");
}

void MainWindow::readServerResponse()
{
    QDataStream in(tcpSocket);
    in.setVersion(QDataStream::Qt_6_0);

    while (tcpSocket->bytesAvailable() > 0) {
        quint16 blockSize;
        if (tcpSocket->bytesAvailable() < sizeof(quint16))
            return;
        in >> blockSize;

        if (tcpSocket->bytesAvailable() < blockSize)
            return;

        quint16 msgTypeRaw;
        in >> msgTypeRaw;
        MessageType type = static_cast<MessageType>(msgTypeRaw);

        QString payload;
        if (in.device()->bytesAvailable() > 0)
            in >> payload;

        qint64 elapsed = requestTimer->elapsed();

        switch (type) {
        case MessageType::S2C_RPN_MATCH_REQUEST_COEFFS:
            appendToOutput("Сервер: ОПЗ совпадает. Отправка коэффициентов...", "green");
            sendCoefficientsToServer();
            break;

        case MessageType::S2C_FINAL_RESULT:
            appendToOutput("Сервер: Результат = " + payload, "magenta");
            addHistoryRecord("Успех", currentExpression, currentClientRPN, payload, elapsed);
            tcpSocket->disconnectFromHost();
            break;

        case MessageType::S2C_RPN_MISMATCH_SEND_CORRECT:
            appendToOutput("Сервер: Ошибка ОПЗ", "red");
            appendToOutput("Правильная ОПЗ: " + payload, "red");
            currentClientRPN = payload;
            addHistoryRecord("Несоответствие ОПЗ", currentExpression, currentClientRPN,
                             "Исправлено: " + payload, elapsed);
            sendInitialRequest();
            break;

        case MessageType::S2C_EXPRESSION_ERROR:
        case MessageType::S2C_CALCULATION_ERROR:
        case MessageType::S2C_PROTOCOL_ERROR:
            appendToOutput("Сервер: Ошибка - " + payload, "red");
            addHistoryRecord("Ошибка", currentExpression, currentClientRPN, payload, elapsed);
            tcpSocket->disconnectFromHost();
            break;

        default:
            appendToOutput("Неизвестный ответ сервера", "red");
            tcpSocket->disconnectFromHost();
            break;
        }
    }
}

void MainWindow::onSocketStateChanged(QAbstractSocket::SocketState socketState)
{
    if (socketState == QAbstractSocket::ConnectedState) {
        appendToOutput("Подключено к серверу", "green");
        sendInitialRequest();
    } else if (socketState == QAbstractSocket::UnconnectedState) {
        appendToOutput("Отключено от сервера", "orange");
    }
}

void MainWindow::onSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    appendToOutput("Ошибка сети: " + tcpSocket->errorString(), "red");
}

bool MainWindow::readExpressionAndOperands()
{
    QFile file(currentFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        appendToOutput("Ошибка открытия файла", "red");
        return false;
    }

    QTextStream in(&file);
    currentExpression = in.readLine().trimmed();
    currentOperands.clear();

    int lineNum = 2;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QStringList parts = line.split('=');
        if (parts.size() != 2) {
            appendToOutput(QString("Ошибка в строке %1: нет '='").arg(lineNum), "red");
            return false;
        }

        QString name = parts[0].trimmed();
        QString valueStr = parts[1].trimmed();

        if (name.isEmpty()) {
            appendToOutput(QString("Ошибка в строке %1: нет имени").arg(lineNum), "red");
            return false;
        }

        bool ok;
        double value = valueStr.toDouble(&ok);
        if (!ok) {
            appendToOutput(QString("Ошибка в строке %1: некорректное значение").arg(lineNum), "red");
            return false;
        }

        currentOperands[name] = value;
        lineNum++;
    }

    file.close();
    return true;
}

void MainWindow::addHistoryRecord(const QString &type,
                                  const QString &expression,
                                  const QString &rpn,
                                  const QString &result,
                                  qint64 time)
{
    HistoryRecord record;
    record.date = Date::now();
    record.type = type;
    record.expression = expression;
    record.rpn = rpn;
    record.result = result;
    record.processingTime = time;
    historyRecords.append(record);
}

int MainWindow::getPrecedence(char op)
{
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    if (op == '^') return 3;
    return 0;
}

bool MainWindow::isOpeningBracket(char c) const {
    return c == '(' || c == '[' || c == '{';
}

bool MainWindow::isClosingBracket(char c) const {
    return c == ')' || c == ']' || c == '}';
}

char MainWindow::getMatchingBracket(char c) const {
    switch(c) {
    case '(': return ')';
    case ')': return '(';
    case '[': return ']';
    case ']': return '[';
    case '{': return '}';
    case '}': return '{';
    default: return '\0';
    }
}

bool MainWindow::isOperator(char c) const {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
}


bool MainWindow::convertToRPNClient(const QString &expression, QString &rpn) {
    std::stack<char> opStack;
    std::string output;
    std::string expr = expression.toStdString();

    for (size_t i = 0; i < expr.length(); ++i) {
        char c = expr[i];
        if (isspace(c)) continue;

        if (isdigit(c) || isalpha(c) || c == '.') {
            while (i < expr.length() && (isdigit(expr[i]) || isalpha(expr[i]) || expr[i] == '.')) {
                output += expr[i++];
            }
            output += ' ';
            i--;
        }
        else if (isOpeningBracket(c)) {
            opStack.push(c);
        }
        else if (isClosingBracket(c)) {
            char matching = getMatchingBracket(c);
            while (!opStack.empty() && opStack.top() != matching) {
                output += opStack.top();
                output += ' ';
                opStack.pop();
            }
            if (opStack.empty()) {
                appendToOutput("Ошибка: Непарные скобки", "red");
                return false;
            }
            opStack.pop();
        }
        else if (isOperator(c)) {
            while (!opStack.empty() && !isOpeningBracket(opStack.top()) &&
                   getPrecedence(opStack.top()) >= getPrecedence(c)) {
                output += opStack.top();
                output += ' ';
                opStack.pop();
            }
            opStack.push(c);
        }
        else {
            appendToOutput(QString("Ошибка: Недопустимый символ '%1'").arg(c), "red");
            return false;
        }
    }
    while (!opStack.empty()) {
        if (isOpeningBracket(opStack.top())) {
            appendToOutput("Ошибка: Непарные скобки", "red");
            return false;
        }
        output += opStack.top();
        output += ' ';
        opStack.pop();
    }

    rpn = QString::fromStdString(output).trimmed();
    return true;
}

void MainWindow::saveToBinaryLog(const QString &expr, const QString &rpn)
{
    QFile file("client_log.bin");
    if (!file.open(QIODevice::Append)) {
        appendToOutput("Ошибка сохранения лога", "red");
        return;
    }

    BinaryLogEntry entry;
    memset(&entry, 0, sizeof(entry));

    QTime now = QTime::currentTime();
    entry.hour = now.hour();
    entry.minute = now.minute();
    entry.second = now.second();

    strncpy(entry.expression, expr.toStdString().c_str(), 79);
    strncpy(entry.rpn, rpn.toStdString().c_str(), 79);

    file.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
    file.close();
}

void MainWindow::saveHistoryToBinary()
{
    QFile file("client_history.bin");
    if (!file.open(QIODevice::WriteOnly)) {
        appendToOutput("Ошибка сохранения истории", "red");
        return;
    }

    struct HistoryBinaryEntry {
        quint16 year;
        quint8 month;
        quint8 day;
        quint8 hour;
        quint8 minute;
        quint8 second;
        char type[32];
        char expression[80];
        char rpn[80];
        char result[80];
        quint32 processingTime;
    };

    for (const auto &record : historyRecords) {
        HistoryBinaryEntry entry;
        memset(&entry, 0, sizeof(entry));

        entry.year = static_cast<quint16>(record.date.getYear());
        entry.month = static_cast<quint8>(record.date.getMonth());
        entry.day = static_cast<quint8>(record.date.getDay());
        entry.hour = static_cast<quint8>(record.date.getHour());
        entry.minute = static_cast<quint8>(record.date.getMinute());
        entry.second = static_cast<quint8>(record.date.getSecond());

        strncpy(entry.type, record.type.toStdString().c_str(), 31);
        strncpy(entry.expression, record.expression.toStdString().c_str(), 79);
        strncpy(entry.rpn, record.rpn.toStdString().c_str(), 79);
        strncpy(entry.result, record.result.toStdString().c_str(), 79);

        entry.processingTime = static_cast<quint32>(record.processingTime);

        file.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
    }

    file.close();
    appendToOutput("История сохранена в client_history.bin", "green");
}

void MainWindow::appendToOutput(const QString &text, const QString &color)
{
    textEdit->append(QString("<span style='color:%1;'>%2</span>").arg(color).arg(text));
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "О программе", "Клиент для вычисления алгебраических выражений");
}

void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Открыть файл", "", "Текстовые файлы (*.txt)");
    if (!fileName.isEmpty()) {
        currentFile = fileName;
        appendToOutput("Открыт файл: " + fileName, "blue");
    }
}

void MainWindow::clearOutput()
{
    textEdit->clear();
}

void MainWindow::showHistory()
{
    if (historyDialog) {
        historyDialog->deleteLater();
    }
    historyDialog = new HistoryDialog(historyRecords, this);
    historyDialog->exec();
}

void MainWindow::saveHistory()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить историю", "", "Текстовые файлы (*.txt)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Дата\tТип\tВыражение\tОПЗ\tРезультат\tВремя (мс)\n";
        for (const auto &record : historyRecords) {
            out << QString::fromStdString(record.date.toString()) << "\t"
                << record.type << "\t"
                << record.expression << "\t"
                << record.rpn << "\t"
                << record.result << "\t"
                << record.processingTime << "\n";
        }
        file.close();
        appendToOutput("История сохранена в " + fileName, "green");
    }
}

void MainWindow::sortHistoryByDate()
{
    std::sort(historyRecords.begin(), historyRecords.end(), [](const auto &a, const auto &b) {
        return a.date > b.date;
    });
    showHistory();
}

void MainWindow::sortHistoryByType()
{
    std::sort(historyRecords.begin(), historyRecords.end(), [](const auto &a, const auto &b) {
        return a.type < b.type;
    });
    showHistory();
}

void MainWindow::sortHistoryByLength()
{
    std::sort(historyRecords.begin(), historyRecords.end(), [](const auto &a, const auto &b) {
        return a.expression.length() > b.expression.length();
    });
    showHistory();
}

void MainWindow::sortHistoryByTime()
{
    std::sort(historyRecords.begin(), historyRecords.end(), [](const auto &a, const auto &b) {
        return a.processingTime > b.processingTime;
    });
    showHistory();
}

HistoryDialog::HistoryDialog(const QList<MainWindow::HistoryRecord> &records, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("История вычислений");
    resize(1000, 600);

    QTableWidget *table = new QTableWidget(this);
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({"Дата", "Тип", "Выражение", "ОПЗ", "Результат", "Время (мс)"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table->setSortingEnabled(true);

    table->setRowCount(records.size());
    for (int i = 0; i < records.size(); ++i) {
        const auto &record = records[i];
        table->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(record.date.toString())));
        table->setItem(i, 1, new QTableWidgetItem(record.type));
        table->setItem(i, 2, new QTableWidgetItem(record.expression));
        table->setItem(i, 3, new QTableWidgetItem(record.rpn));
        table->setItem(i, 4, new QTableWidgetItem(record.result));
        table->setItem(i, 5, new QTableWidgetItem(QString::number(record.processingTime)));
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(table);
    layout->addWidget(buttonBox);
}
