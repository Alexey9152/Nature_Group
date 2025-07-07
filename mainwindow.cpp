#include "mainwindow.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QTableWidget>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QElapsedTimer>
#include <stack>

HistoryDialog::HistoryDialog(const QList<MainWindow::HistoryRecord> &records, QWidget *parent)
    : QDialog(parent), records(records)
{
    setWindowTitle("История вычислений");
    resize(800, 600);

    table = new QTableWidget(this);
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({"Дата", "Тип", "Выражение", "ОПЗ", "Результат", "Время (мс)"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(table);
    layout->addWidget(buttonBox);

    populateTable();
}

void HistoryDialog::populateTable()
{
    table->setRowCount(records.size());
    for (int i = 0; i < records.size(); ++i) {
        const auto &record = records[i];
        std::ostringstream dateStream;
        dateStream << record.date;
        QString date = QString::fromStdString(dateStream.str());

        table->setItem(i, 0, new QTableWidgetItem(date));
        table->setItem(i, 1, new QTableWidgetItem(record.type));
        table->setItem(i, 2, new QTableWidgetItem(record.expression));
        table->setItem(i, 3, new QTableWidgetItem(record.rpn));
        table->setItem(i, 4, new QTableWidgetItem(record.result));
        table->setItem(i, 5, new QTableWidgetItem(QString::number(record.processingTime)));
    }
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Калькулятор алгебраического выражения");
    resize(800, 600);

    textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    runButton = new QPushButton("Run Program", this);
    aboutButton = new QPushButton("About", this);
    openButton = new QPushButton("Open File", this);
    clearButton = new QPushButton("Clear Output", this);
    tcpSocket = new QTcpSocket(this);

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

    connect(runButton, &QPushButton::clicked, this, &MainWindow::runProgram);
    connect(aboutButton, &QPushButton::clicked, this, &MainWindow::showAbout);
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openFile);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearOutput);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::readServerResponse);
    connect(tcpSocket, &QTcpSocket::errorOccurred, this, [this]() {
        appendToOutput("Network error: " + tcpSocket->errorString(), "red");
    });
    connect(tcpSocket, &QTcpSocket::connected, this, [this]() {
        appendToOutput("Connected to server.", "green");
    });
    connect(tcpSocket, &QTcpSocket::disconnected, this, [this]() {
        appendToOutput("Disconnected from server.", "orange");
    });

    QMenu *historyMenu = menuBar()->addMenu("История");
    QAction *showHistoryAction = historyMenu->addAction("Показать историю");
    QAction *saveHistoryAction = historyMenu->addAction("Сохранить историю");
    QAction *serverHistoryAction = historyMenu->addAction("История сервера");

    QMenu *sortMenu = menuBar()->addMenu("Сортировка");
    QAction *sortByDateAction = sortMenu->addAction("По дате");
    QAction *sortByTypeAction = sortMenu->addAction("По типу запроса");
    QAction *sortByLengthAction = sortMenu->addAction("По длине выражения");
    QAction *sortByTimeAction = sortMenu->addAction("По времени обработки");

    connect(showHistoryAction, &QAction::triggered, this, &MainWindow::showHistory);
    connect(saveHistoryAction, &QAction::triggered, this, &MainWindow::saveHistory);
    connect(serverHistoryAction, &QAction::triggered, this, &MainWindow::showServerHistory);
    connect(sortByDateAction, &QAction::triggered, this, &MainWindow::sortHistoryByDate);
    connect(sortByTypeAction, &QAction::triggered, this, &MainWindow::sortHistoryByType);
    connect(sortByLengthAction, &QAction::triggered, this, &MainWindow::sortHistoryByLength);
    connect(sortByTimeAction, &QAction::triggered, this, &MainWindow::sortHistoryByTime);

    appendToOutput("Все готово к запуску", "cyan");
    appendToOutput("Нажмите 'Open File' чтобы выбрать файл и 'Run Program' чтобы начать его обработку", "cyan");
}

MainWindow::~MainWindow()
{
    delete historyDialog;
}

void MainWindow::appendToOutput(const QString& text, const QString& color)
{
    textEdit->append(QString("<span style='color:%1;'>%2</span>").arg(color).arg(text));
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "About", "Calculator Application");
}

void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open File");
    if (!fileName.isEmpty()) {
        currentFile = fileName;
        appendToOutput("Выбран файл: " + fileName, "blue");
    }
}

void MainWindow::clearOutput()
{
    textEdit->clear();
}

void MainWindow::runProgram()
{
    if (currentFile.isEmpty()) {
        appendToOutput("ERROR: Файл не выбран. Нажмите 'Open File'", "red");
        return;
    }

    textEdit->clear();
    appendToOutput("Обработка файла: " + currentFile, "white");

    currentExpression.clear();
    currentOperands.clear();

    if (!readExpressionAndOperands(currentExpression, currentOperands)) {
        appendToOutput("Обработка файла остановлена из-за ошибок", "red");
        return;
    }

    appendToOutput("Выражение: " + currentExpression, "blue");
    for (const auto& [name, value] : currentOperands) {
        appendToOutput(QString("Операнд: %1 = %2").arg(QString::fromStdString(name)).arg(value), "blue");
    }

    if (!convertToRPNClient(currentExpression, currentClientRPN)) {
        appendToOutput("ERROR: Ошибка при построении ОПЗ на клиенте.", "red");
        return;
    }
    appendToOutput("ОПЗ, сгенерированная клиентом: " + currentClientRPN, "blue");

    requestTimer.start();
    sendExpressionAndRPNToServer(currentExpression, currentClientRPN);
}

bool MainWindow::readExpressionAndOperands(QString &expression, std::map<std::string, double> &operands)
{
    QFile file(currentFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        appendToOutput("ERROR: Не удалось открыть файл", "red");
        return false;
    }

    QTextStream in(&file);
    expression = in.readLine();

    int lineNum = 2;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QStringList parts = line.split('=');
        if (parts.size() != 2) {
            appendToOutput(QString("ERROR: Строка %1 - пропуск '='").arg(lineNum), "red");
            return false;
        }

        QString name = parts[0].trimmed();
        QString valueStr = parts[1].trimmed();

        if (name.isEmpty()) {
            appendToOutput(QString("ERROR: Строка %1 - отсутствует имя операнда").arg(lineNum), "red");
            return false;
        }

        bool ok;
        double value = valueStr.toDouble(&ok);
        if (!ok) {
            appendToOutput(QString("ERROR: Строка %1 - некорректное значение: '%2'")
                               .arg(lineNum).arg(valueStr), "red");
            return false;
        }

        operands[name.toStdString()] = value;
        lineNum++;
    }

    file.close();
    return true;
}

void MainWindow::sendExpressionAndRPNToServer(const QString& expression, const QString& rpn)
{
    QString ip = ipEdit->text();
    quint16 port = static_cast<quint16>(portSpin->value());

    tcpSocket->connectToHost(ip, port);
    if (!tcpSocket->waitForConnected(3000)) {
        appendToOutput("Could not connect to server", "red");
        return;
    }

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << quint16(0) << expression << rpn;
    out.device()->seek(0);
    out << quint16(block.size() - sizeof(quint16));

    tcpSocket->write(block);
    appendToOutput("Отправлено выражение и ОПЗ на сервер.", "blue");
}

void MainWindow::readServerResponse()
{
    QDataStream in(tcpSocket);
    in.setVersion(QDataStream::Qt_6_0);

    if (tcpSocket->bytesAvailable() < sizeof(quint16))
        return;

    quint16 blockSize;
    in >> blockSize;

    if (tcpSocket->bytesAvailable() < blockSize)
        return;

    QString response;
    in >> response;

    qint64 elapsed = requestTimer.elapsed();
    addHistoryRecord("Ответ сервера", currentExpression, currentClientRPN, response, elapsed);
    appendToOutput("Ответ сервера: " + response, "green");
}

void MainWindow::addHistoryRecord(const QString &type, const QString &expression,
                                  const QString &rpn, const QString &result, qint64 time)
{
    HistoryRecord record;
    QDateTime now = QDateTime::currentDateTime();
    record.date = Date(now.date().day(), now.date().month(), now.date().year(),
                       now.time().hour(), now.time().minute(), now.time().second());
    record.type = type;
    record.expression = expression;
    record.rpn = rpn;
    record.result = result;
    record.processingTime = time;
    historyRecords.append(record);
}

int MainWindow::getPrecedence(char op) {
    switch (op) {
    case '^': return 4;
    case '*':
    case '/': return 3;
    case '+':
    case '-': return 2;
    default: return 0;
    }
}

bool MainWindow::isOperator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
}

bool MainWindow::convertToRPNClient(const QString& expression, QString& rpn)
{
    std::stack<char> opStack;
    std::string output;
    std::string expr = expression.toStdString();

    for (size_t i = 0; i < expr.length(); ++i) {
        char c = expr[i];

        // Пропускаем пробелы
        if (c == ' ') continue;

        // Обработка чисел (целых и с плавающей точкой)
        if (isdigit(c) || c == '.') {
            while (i < expr.length() && (isdigit(expr[i]) || expr[i] == '.')) {
                output += expr[i++];
            }
            output += ' ';
            i--;
            continue;
        }

        // Обработка переменных (идентификаторов)
        if (isalpha(c)) {
            while (i < expr.length() && isalnum(expr[i])) {
                output += expr[i++];
            }
            output += ' ';
            i--;
            continue;
        }

        // Обработка открывающих скобок
        if (c == '(' || c == '[' || c == '{') {
            opStack.push(c);
            continue;
        }

        // Обработка закрывающих скобок
        if (c == ')' || c == ']' || c == '}') {
            char matchingBracket;
            switch (c) {
            case ')': matchingBracket = '('; break;
            case ']': matchingBracket = '['; break;
            case '}': matchingBracket = '{'; break;
            }

            while (!opStack.empty() && opStack.top() != matchingBracket) {
                output += opStack.top();
                output += ' ';
                opStack.pop();
            }

            if (opStack.empty()) {
                appendToOutput("Ошибка: Несоответствующие скобки", "red");
                return false;
            }

            opStack.pop(); // Удаляем соответствующую открывающую скобку
            continue;
        }

        // Обработка операторов
        if (isOperator(c)) {
            while (!opStack.empty() &&
                   opStack.top() != '(' && opStack.top() != '[' && opStack.top() != '{' &&
                   getPrecedence(opStack.top()) >= getPrecedence(c)) {
                output += opStack.top();
                output += ' ';
                opStack.pop();
            }
            opStack.push(c);
            continue;
        }

        // Если дошли сюда - недопустимый символ
        appendToOutput(QString("Ошибка: Недопустимый символ '%1'").arg(c), "red");
        return false;
    }

    // Выталкиваем оставшиеся операторы из стека
    while (!opStack.empty()) {
        if (opStack.top() == '(' || opStack.top() == '[' || opStack.top() == '{') {
            appendToOutput("Ошибка: Несоответствующие скобки", "red");
            return false;
        }
        output += opStack.top();
        output += ' ';
        opStack.pop();
    }

    rpn = QString::fromStdString(output).trimmed();
    return true;
}

void MainWindow::showHistory()
{
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
            std::ostringstream dateStream;
            dateStream << record.date;
            out << QString::fromStdString(dateStream.str()) << "\t"
                << record.type << "\t"
                << record.expression << "\t"
                << record.rpn << "\t"
                << record.result << "\t"
                << record.processingTime << "\n";
        }
        file.close();
    }
}

void MainWindow::showServerHistory()
{
    QMessageBox::information(this, "История сервера",
                             "История сервера будет отображена здесь.");
}

void MainWindow::sortHistoryByDate()
{
    std::sort(historyRecords.begin(), historyRecords.end(),
              [](const HistoryRecord &a, const HistoryRecord &b) {
                  return a.date > b.date;
              });
    showHistory();
}

void MainWindow::sortHistoryByType()
{
    std::sort(historyRecords.begin(), historyRecords.end(),
              [](const HistoryRecord &a, const HistoryRecord &b) {
                  return a.type < b.type;
              });
    showHistory();
}

void MainWindow::sortHistoryByLength()
{
    std::sort(historyRecords.begin(), historyRecords.end(),
              [](const HistoryRecord &a, const HistoryRecord &b) {
                  return a.expression.length() > b.expression.length();
              });
    showHistory();
}

void MainWindow::sortHistoryByTime()
{
    std::sort(historyRecords.begin(), historyRecords.end(),
              [](const HistoryRecord &a, const HistoryRecord &b) {
                  return a.processingTime > b.processingTime;
              });
    showHistory();
}
