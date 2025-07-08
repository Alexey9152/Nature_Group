#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QElapsedTimer>
#include <stack>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setupUi();
    setupMenu();

    tcpSocket = new QTcpSocket(this);
    requestTimer = new QElapsedTimer();

    connect(runButton, &QPushButton::clicked, this, &MainWindow::runProgram);
    connect(aboutButton, &QPushButton::clicked, this, &MainWindow::showAbout);
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openFile);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearOutput);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::readServerResponse);
    connect(tcpSocket, &QTcpSocket::stateChanged, this, &MainWindow::onSocketStateChanged);
    connect(tcpSocket, &QAbstractSocket::errorOccurred, this, &MainWindow::onSocketError);

    appendToOutput("Все готово к запуску.", "cyan");
    appendToOutput("Нажмите 'Open File' чтобы выбрать файл и 'Run Program' чтобы начать его обработку.", "cyan");
}

MainWindow::~MainWindow()
{
    delete requestTimer;
}

void MainWindow::setupUi() {
    setWindowTitle("Калькулятор алгебраического выражения");
    resize(800, 600);

    textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    runButton = new QPushButton("Run Program", this);
    aboutButton = new QPushButton("About", this);
    openButton = new QPushButton("Open File", this);
    clearButton = new QPushButton("Clear Output", this);
    
    ipEdit = new QLineEdit("localhost", this);
    portSpin = new QSpinBox(this);
    portSpin->setRange(1, 65535);
    portSpin->setValue(12345);

    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    QHBoxLayout* serverLayout = new QHBoxLayout();
    serverLayout->addWidget(new QLabel("Server IP:", this));
    serverLayout->addWidget(ipEdit);
    serverLayout->addWidget(new QLabel("Port:", this));
    serverLayout->addWidget(portSpin);
    mainLayout->addLayout(serverLayout);
    mainLayout->addWidget(textEdit);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(openButton);
    buttonLayout->addWidget(runButton);
    buttonLayout->addWidget(aboutButton);
    buttonLayout->addWidget(clearButton);
    mainLayout->addLayout(buttonLayout);
    setCentralWidget(centralWidget);
}

void MainWindow::setupMenu() {
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
        appendToOutput("ERROR: Файл не выбран. Нажмите 'Open File'", "red");
        return;
    }
    textEdit->clear();
    appendToOutput("Обработка файла: " + currentFile, "white");
    if (!readExpressionAndOperands()) {
        appendToOutput("Обработка файла остановлена из-за ошибок.", "red"); return;
    }
    if (!convertToRPNClient(currentExpression, currentClientRPN)) {
        appendToOutput("ERROR: Ошибка при построении ОПЗ на клиенте.", "red"); return;
    }

    appendToOutput("Выражение: " + currentExpression, "blue");
    appendToOutput("ОПЗ, сгенерированная клиентом: " + currentClientRPN, "blue");
    for (auto it = currentOperands.constBegin(); it != currentOperands.constEnd(); ++it) {
        appendToOutput(QString("Операнд: %1 = %2").arg(it.key()).arg(it.value()), "blue");
    }

    requestTimer->start();
    appendToOutput("Connecting to server...", "gray");
    tcpSocket->connectToHost(ipEdit->text(), static_cast<quint16>(portSpin->value()));
}

void MainWindow::sendInitialRequest()
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << quint16(0) << static_cast<quint16>(MessageType::C2S_EXPRESSION_SUBMISSION)
        << currentExpression << currentClientRPN;
    out.device()->seek(0);
    out << quint16(block.size() - sizeof(quint16));
    tcpSocket->write(block);
    appendToOutput("Отправлен первоначальный запрос на сервер.", "blue");
}

void MainWindow::sendCoefficientsToServer()
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << quint16(0) << static_cast<quint16>(MessageType::C2S_COEFFICIENTS_SUBMISSION)
        << currentOperands;
    out.device()->seek(0);
    out << quint16(block.size() - sizeof(quint16));
    tcpSocket->write(block);
    appendToOutput("Коэффициенты отправлены на сервер.", "blue");
}

void MainWindow::readServerResponse()
{
    QDataStream in(tcpSocket);
    in.setVersion(QDataStream::Qt_6_0);

    while(tcpSocket->bytesAvailable() > 0) {
        quint16 blockSize;
        if(tcpSocket->bytesAvailable() < sizeof(quint16)) return;
        in >> blockSize;
        if(tcpSocket->bytesAvailable() < blockSize) return;

        quint16 msgTypeRaw;
        in >> msgTypeRaw;
        MessageType type = static_cast<MessageType>(msgTypeRaw);

        QString payload;
        if (in.device()->bytesAvailable() > 0) in >> payload;
        qint64 elapsed = requestTimer->elapsed();

        switch(type) {
            case MessageType::S2C_RPN_MATCH_REQUEST_COEFFS:
                appendToOutput("ОТВЕТ СЕРВЕРА: ОПЗ верна. Отправка коэффициентов...", "green");
                sendCoefficientsToServer();
                break;
            case MessageType::S2C_FINAL_RESULT:
                appendToOutput("ОТВЕТ СЕРВЕРА: Итоговый результат = " + payload, "magenta");
                addHistoryRecord("Success", currentExpression, currentClientRPN, payload, elapsed);
                tcpSocket->disconnectFromHost();
                break;
            case MessageType::S2C_RPN_MISMATCH_SEND_CORRECT:
                appendToOutput("ОТВЕТ СЕРВЕРА: ОШИБКА! ОПЗ не совпала.", "red");
                appendToOutput("Сервер считает, что правильная ОПЗ: " + payload, "red");
                addHistoryRecord("RPN Mismatch", currentExpression, currentClientRPN, "Server RPN: " + payload, elapsed);
                tcpSocket->disconnectFromHost();
                break;
            case MessageType::S2C_EXPRESSION_ERROR:
            case MessageType::S2C_CALCULATION_ERROR:
            case MessageType::S2C_PROTOCOL_ERROR:
                appendToOutput("ОТВЕТ СЕРВЕРА: ОШИБКА - " + payload, "red");
                addHistoryRecord("Server Error", currentExpression, currentClientRPN, payload, elapsed);
                tcpSocket->disconnectFromHost();
                break;
            default:
                appendToOutput("Получен неизвестный ответ от сервера.", "red");
                tcpSocket->disconnectFromHost();
                break;
        }
    }
}

void MainWindow::onSocketStateChanged(QAbstractSocket::SocketState socketState)
{
    if (socketState == QAbstractSocket::ConnectedState) {
        appendToOutput("Connected to server.", "green");
        sendInitialRequest();
    } else if (socketState == QAbstractSocket::UnconnectedState) {
        appendToOutput("Disconnected from server.", "orange");
    }
}

void MainWindow::onSocketError(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError);
    appendToOutput("Network error: " + tcpSocket->errorString(), "red");
}

bool MainWindow::readExpressionAndOperands()
{
    QFile file(currentFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        appendToOutput("ERROR: Не удалось открыть файл", "red"); return false;
    }
    QTextStream in(&file);
    currentExpression = in.readLine();
    currentOperands.clear();
    int lineNum = 2;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        QStringList parts = line.split('=');
        if (parts.size() != 2) { appendToOutput(QString("ERROR: Строка %1 - пропуск '='").arg(lineNum), "red"); return false; }
        QString name = parts[0].trimmed();
        QString valueStr = parts[1].trimmed();
        if (name.isEmpty()) { appendToOutput(QString("ERROR: Строка %1 - отсутствует имя операнда").arg(lineNum), "red"); return false; }
        bool ok;
        double value = valueStr.toDouble(&ok);
        if (!ok) { appendToOutput(QString("ERROR: Строка %1 - некорректное значение: '%2'").arg(lineNum).arg(valueStr), "red"); return false; }
        currentOperands[name] = value;
        lineNum++;
    }
    file.close();
    return true;
}

void MainWindow::addHistoryRecord(const QString &type, const QString &expression, const QString &rpn, const QString &result, qint64 time)
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

int MainWindow::getPrecedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    if (op == '^') return 3;
    return 0;
}

bool MainWindow::isOperator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
}

bool MainWindow::convertToRPNClient(const QString& expression, QString& rpn) {
    std::stack<char> opStack;
    std::string output;
    std::string expr = expression.toStdString();
    for (size_t i = 0; i < expr.length(); ++i) {
        char c = expr[i];
        if (isspace(c)) continue;
        if (isdigit(c) || (c == '.')) {
            while (i < expr.length() && (isdigit(expr[i]) || expr[i] == '.')) output += expr[i++];
            output += ' '; i--;
        } else if (isalpha(c)) {
            while (i < expr.length() && isalnum(expr[i])) output += expr[i++];
            output += ' '; i--;
        } else if (c == '(') {
            opStack.push(c);
        } else if (c == ')') {
            while (!opStack.empty() && opStack.top() != '(') {
                output += opStack.top(); output += ' '; opStack.pop();
            }
            if (opStack.empty()) { appendToOutput("Ошибка: Несоответствующие скобки", "red"); return false; }
            opStack.pop();
        } else if (isOperator(c)) {
            while (!opStack.empty() && opStack.top() != '(' && getPrecedence(opStack.top()) >= getPrecedence(c)) {
                output += opStack.top(); output += ' '; opStack.pop();
            }
            opStack.push(c);
        } else {
            appendToOutput(QString("Ошибка: Недопустимый символ '%1'").arg(c), "red"); return false;
        }
    }
    while (!opStack.empty()) {
        if (opStack.top() == '(') { appendToOutput("Ошибка: Несоответствующие скобки", "red"); return false; }
        output += opStack.top(); output += ' '; opStack.pop();
    }
    rpn = QString::fromStdString(output).trimmed();
    return true;
}

void MainWindow::appendToOutput(const QString& text, const QString& color) { textEdit->append(QString("<span style='color:%1;'>%2</span>").arg(color).arg(text)); }
void MainWindow::showAbout() { QMessageBox::about(this, "About", "Calculator Application"); }
void MainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open File", "", "Text files (*.txt)");
    if (!fileName.isEmpty()) { currentFile = fileName; appendToOutput("Выбран файл: " + fileName, "blue"); }
}
void MainWindow::clearOutput() { textEdit->clear(); }
void MainWindow::showHistory() {
    if (historyDialog) { historyDialog->deleteLater(); }
    historyDialog = new HistoryDialog(historyRecords, this);
    historyDialog->exec();
}
void MainWindow::saveHistory() {
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить историю", "", "Текстовые файлы (*.txt)");
    if (fileName.isEmpty()) return;
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Дата\tТип\tВыражение\tОПЗ\tРезультат\tВремя (мс)\n";
        for (const auto &record : historyRecords) {
            out << QString::fromStdString(record.date.toString()) << "\t"
                << record.type << "\t" << record.expression << "\t"
                << record.rpn << "\t" << record.result << "\t"
                << record.processingTime << "\n";
        }
        file.close();
    }
}
void MainWindow::sortHistoryByDate() {
    std::sort(historyRecords.begin(), historyRecords.end(), [](const auto &a, const auto &b) { return a.date > b.date; }); showHistory();
}
void MainWindow::sortHistoryByType() {
    std::sort(historyRecords.begin(), historyRecords.end(), [](const auto &a, const auto &b) { return a.type < b.type; }); showHistory();
}
void MainWindow::sortHistoryByLength() {
    std::sort(historyRecords.begin(), historyRecords.end(), [](const auto &a, const auto &b) { return a.expression.length() > b.expression.length(); }); showHistory();
}
void MainWindow::sortHistoryByTime() {
    std::sort(historyRecords.begin(), historyRecords.end(), [](const auto &a, const auto &b) { return a.processingTime > b.processingTime; }); showHistory();
}

HistoryDialog::HistoryDialog(const QList<MainWindow::HistoryRecord> &records, QWidget *parent) : QDialog(parent) {
    setWindowTitle("История вычислений");
    resize(800, 600);
    QTableWidget *table = new QTableWidget(this);
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({"Дата", "Тип", "Выражение", "ОПЗ", "Результат", "Время (мс)"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(table);
    layout->addWidget(buttonBox);
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
}
