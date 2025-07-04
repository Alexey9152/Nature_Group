#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Калькулятор алгебраического выражения");
    resize(800, 600);

    // Виджеты
    textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    runButton = new QPushButton("Run Program", this);
    aboutButton = new QPushButton("About", this);
    openButton = new QPushButton("Open File", this);
    clearButton = new QPushButton("Clear Output", this);
    tcpSocket = new QTcpSocket(this);

    // Добавляем поля для IP и порта
    ipEdit = new QLineEdit("localhost", this);
    portSpin = new QSpinBox(this);
    portSpin->setRange(1, 65535);
    portSpin->setValue(12345);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Строка с настройками сервера
    QHBoxLayout *serverLayout = new QHBoxLayout();
    serverLayout->addWidget(new QLabel("Server IP:", this));
    serverLayout->addWidget(ipEdit);
    serverLayout->addWidget(new QLabel("Port:", this));
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
    connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::readServerResponse);
    connect(tcpSocket, &QTcpSocket::errorOccurred, this, [this]() {
        appendToOutput("Network error: " + tcpSocket->errorString(), "red");
    });

    appendToOutput("Все готово к запуску", "cyan");
    appendToOutput("Нажмите 'Open File' чтобы выбрать файл и 'Run Program' чтобы начать его обработку", "cyan");
}

void MainWindow::appendToOutput(const QString &text, const QString &color)
{
    textEdit->append(QString("<span style='color:%1;'>%2</span>").arg(color).arg(text));
}

void MainWindow::showAbout()
{
    QString aboutText = "ЭТА ПРОГРАММА ПРОВЕРЯЕТ ОШИБКИ ПРИ ЗАПИСИ ВЫРАЖЕНИЙ\n"
                        "ИЗ ФАЙЛА, ПРЕОБРАЗУЕТ В ОБРАТНУЮ ПОЛЬСКУЮ ЗАПИСЬ,\n"
                        "ВЫЧИСЛЯЕТ ЗНАЧЕНИЕ ВЫРАЖЕНИЯ\n\n"
                        "Требования к формату файла:\n"
                        "- Первая строка: выражение\n"
                        "- Последующие строки: определения операндов (имя = значение)\n"
                        "- Имена операндов не могут быть числами и не должны содержать '='\n\n"
                        "Поддерживаемые операции: +, -, *, /\n"
                        "Поддерживаемые скобки: (), [], {}";

    QMessageBox::information(this, "About Program", aboutText);
}

void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Открыть файл ввода",
                                                    "",
                                                    "Текстовые файлы (*.txt);;Все файлы (*.*)");
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

    std::queue<char> expression;
    if (!readExpression(expression)) {
        appendToOutput("Обработка файла остановлена из-за ошибок", "red");
        return;
    }

    QString exprStr;
    while (!expression.empty()) {
        exprStr += QChar(expression.front());
        expression.pop();
    }

    std::ifstream in(currentFile.toStdString());
    std::string line;
    std::getline(in, line); // Пропускаем первую строку (выражение)

    std::map<std::string, double> operands;
    int lineNum = 2;
    while (std::getline(in, line)) {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

        if (line.empty())
            continue;

        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            appendToOutput(QString("ERROR: Строка %1 - пропуск '='").arg(lineNum), "red");
            return;
        }

        std::string name = line.substr(0, pos);
        size_t start = name.find_first_not_of(" \t");
        size_t end = name.find_last_not_of(" \t");
        if (start == std::string::npos) {
            appendToOutput(QString("ERROR: Строка %1 - отсутствует имя операнда").arg(lineNum), "red");
            return;
        }
        name = name.substr(start, end - start + 1);

        if (std::all_of(name.begin(), name.end(), [](char c) { return std::isdigit(c); })) {
            appendToOutput(QString("ERROR: Строка %1 - имя операнда не может быть числом: '%2'")
                               .arg(lineNum)
                               .arg(QString::fromStdString(name)), "red");
            return;
        }

        std::string valueStr = line.substr(pos + 1);
        start = valueStr.find_first_not_of(" \t");
        if (start == std::string::npos) {
            appendToOutput(QString("ERROR: Строка %1 - пропущено значение операнда").arg(lineNum), "red");
            return;
        }
        valueStr = valueStr.substr(start);

        std::replace(valueStr.begin(), valueStr.end(), ',', '.');

        try {
            double value = std::stod(valueStr);
            operands[name] = value;
            appendToOutput(QString("Операнд: %1 = %2").arg(QString::fromStdString(name)).arg(value), "blue");
        } catch (const std::exception &) {
            appendToOutput(QString("ERROR: Строка %1 - некорректное значение: '%2'")
                               .arg(lineNum)
                               .arg(QString::fromStdString(valueStr)), "red");
            return;
        }

        lineNum++;
    }

    sendToServer(exprStr, operands);
}

bool MainWindow::readExpression(std::queue<char> &expression)
{
    appendToOutput("\nЧтение выражения из файла...", "cyan");

    std::ifstream in(currentFile.toStdString());
    if (!in.is_open()) {
        appendToOutput("ERROR: Файл не открыт", "red");
        return false;
    }

    std::string line;
    if (!std::getline(in, line)) {
        appendToOutput("ERROR: Файл пуст", "red");
        return false;
    }

    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

    for (char c : line) {
        expression.push(c);
    }

    appendToOutput("Выражение успешно прочитано:", "cyan");
    appendToOutput(QString::fromStdString(line), "white");
    return true;
}

void MainWindow::sendToServer(const QString& expression, const std::map<std::string, double>& operands)
{
    QString ip = ipEdit->text();
    quint16 port = static_cast<quint16>(portSpin->value());

    tcpSocket->connectToHost(ip, port);
    if (!tcpSocket->waitForConnected(3000)) {
        appendToOutput("Could not connect to server", "red");
        return;
    }

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);

    out << quint32(0); // Placeholder for size

    out << expression;
    out << static_cast<int>(operands.size());

    for (const auto& [name, value] : operands) {
        out << QString::fromStdString(name) << value;
    }

    out.device()->seek(0);
    out << quint32(data.size() - sizeof(quint32));

    tcpSocket->write(data);
    appendToOutput("Expression sent to server", "blue");
}

void MainWindow::readServerResponse()
{
    QDataStream in(tcpSocket);
    in.setVersion(QDataStream::Qt_6_0);

    quint32 status;
    in >> status;

    if (status == 0) {
        double result;
        in >> result;
        QString resultStr = formatDouble(result);
        appendToOutput("\nРезультат: " + resultStr, "white");
    } else {
        QString errorMsg;
        in >> errorMsg;
        appendToOutput("Server error: " + errorMsg, "red");
    }

    tcpSocket->disconnectFromHost();
}

QString MainWindow::formatDouble(double value)
{
    QString result = QString::number(value, 'g', 10);
    result.replace('.', ',');

    if (result.contains(',') && result.split(',')[1].toDouble() == 0) {
        result = result.split(',')[0];
    }
    return result;
}
