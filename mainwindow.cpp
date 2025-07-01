#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <fstream>
#include <sstream>
#include <cctype>
#include <QFileDialog>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Калькулятор алгебралического выражения");
    resize(800, 600);

    // Виджеты
    textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    runButton = new QPushButton("Run Program", this);
    aboutButton = new QPushButton("About", this);
    openButton = new QPushButton("Open File", this);
    clearButton = new QPushButton("Clear Output", this);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
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

    appendToOutput("Все готово к запуску", "cyan");
    appendToOutput("Нажмите 'Open File' чтобы выбрать файл и 'Run Program' чтобы начать его обработку", "cyan");
}

void MainWindow::appendToOutput(const QString& text, const QString& color)
{
    textEdit->append(QString("<span style='color:%1;'>%2</span>").arg(color).arg(text));
}

void MainWindow::showAbout()
{
    QString aboutText =
        "ЭТА ПРОГРАММА ПРОВЕРЯЕТ ОШИБКИ ПРИ ЗАПИСИ ВЫРАЖЕНИЙ\n"
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
    QString fileName = QFileDialog::getOpenFileName(this, "Open Input File", "", "Text Files (*.txt)");
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

    std::string RPN;
    if (!convertToRPN(expression, RPN)) {
        appendToOutput("Обработка файла остановлена из-за ошибок", "red");
        return;
    }

    appendToOutput("Выражение конвентировано в ОПЗ:", "green");
    appendToOutput(QString::fromStdString(RPN), "white");

    std::map<std::string, double> operands;
    if (!calculateRPN(RPN, operands)) {
        appendToOutput("Обработка файла остановлена из-за ошибок", "red");
    }
}

bool MainWindow::readExpression(std::queue<char>& expression)
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

    // Remove carriage returns for Windows compatibility
    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

    for (char c : line) {
        expression.push(c);
    }

    appendToOutput("Выражение успешно прочитано:", "cyan");
    appendToOutput(QString::fromStdString(line), "white");
    return true;
}

bool MainWindow::convertToRPN(std::queue<char>& expression, std::string& B)
{
    appendToOutput("\nПреобразование в ОПЗ и проверка на ошибки...", "cyan");

    char a;
    std::stack<char> stack1;
    bool indicator = true;
    char pred = 0;
    QString errorMessage;
    QString expressionOutput;

    while (!expression.empty() && indicator) {
        a = expression.front();
        expression.pop();
        expressionOutput += a;

        if (a == '(' || a == '[' || a == '{') {
            if (pred != '+' && pred != '-' && pred != '*' && pred != '/' &&
                pred != '(' && pred != '[' && pred != '{' && pred != 0) {
                errorMessage = QString("ОШИБКА: Отсутствует оператор перед '%1'").arg(a);
                indicator = false;
            }
            else {
                stack1.push(a);
            }
        }
        else if ((a == '+' || a == '-' || a == '*' || a == '/') && expression.empty()) {
            errorMessage = QString("ОШИБКА: Отсутствует правый операнд после '%1'").arg(a);
            indicator = false;
        }
        else if (a == ')') {
            if (pred == '(') {
                errorMessage = "ОШИБКА: Пустые скобки ()";
                indicator = false;
            }
            else {
                if (stack1.empty()) {
                    errorMessage = "ОШИБКА: Несоответствие скобок - нет открывающей скобки для )";
                    indicator = false;
                }
                else {
                    while (stack1.top() != '(' && indicator) {
                        if (stack1.empty()) {
                            errorMessage = "ОШИБКА: Несоответствие скобок - нет открывающей скобки для )";
                            indicator = false;
                        }
                        else if (pred == '-' || pred == '+' || pred == '*' || pred == '/') {
                            errorMessage = "ОШИБКА: Отсутствует правый операнд";
                            indicator = false;
                        }
                        else if (stack1.top() == '[') {
                            errorMessage = "ОШИБКА: Незакрытая квадратная скобка [";
                            indicator = false;
                        }
                        else if (stack1.top() == '{') {
                            errorMessage = "ОШИБКА: Незакрытая фигурная скобка {";
                            indicator = false;
                        }
                        else {
                            B.push_back(stack1.top());
                            B.push_back(' ');
                            stack1.pop();
                        }
                    }
                    if (indicator) stack1.pop();
                }
            }
        }
        else if (a == ']') {
            if (pred == '[') {
                errorMessage = "ОШИБКА: Пустые скобки []";
                indicator = false;
            }
            else {
                if (stack1.empty()) {
                    errorMessage = "ОШИБКА: Несоответствие скобок - нет открывающей скобки для ]";
                    indicator = false;
                }
                else {
                    while (stack1.top() != '[' && indicator) {
                        if (stack1.empty()) {
                            errorMessage = "ОШИБКА: Несоответствие скобок - нет открывающей скобки для ]";
                            indicator = false;
                        }
                        else if (pred == '-' || pred == '+' || pred == '*' || pred == '/') {
                            errorMessage = "ОШИБКА: Отсутствует правый операнд";
                            indicator = false;
                        }
                        else if (stack1.top() == '(') {
                            errorMessage = "ОШИБКА: Незакрытая круглая скобка (";
                            indicator = false;
                        }
                        else if (stack1.top() == '{') {
                            errorMessage = "ОШИБКА: Незакрытая фигурная скобка {";
                            indicator = false;
                        }
                        else {
                            B.push_back(stack1.top());
                            B.push_back(' ');
                            stack1.pop();
                        }
                    }
                    if (indicator) stack1.pop();
                }
            }
        }
        else if (a == '}') {
            if (pred == '{') {
                errorMessage = "ОШИБКА: Пустые скобки {}";
                indicator = false;
            }
            else {
                if (stack1.empty()) {
                    errorMessage = "ОШИБКА: Несоответствие скобок - нет открывающей скобки для }";
                    indicator = false;
                }
                else {
                    while (stack1.top() != '{' && indicator) {
                        if (stack1.empty()) {
                            errorMessage = "ОШИБКА: Несоответствие скобок - нет открывающей скобки для }";
                            indicator = false;
                        }
                        else if (pred == '-' || pred == '+' || pred == '*' || pred == '/') {
                            errorMessage = "ОШИБКА: Отсутствует правый операнд";
                            indicator = false;
                        }
                        else if (stack1.top() == '[') {
                            errorMessage = "ОШИБКА: Незакрытая квадратная скобка [";
                            indicator = false;
                        }
                        else if (stack1.top() == '(') {
                            errorMessage = "ОШИБКА: Незакрытая круглая скобка (";
                            indicator = false;
                        }
                        else {
                            B.push_back(stack1.top());
                            B.push_back(' ');
                            stack1.pop();
                        }
                    }
                    if (indicator) stack1.pop();
                }
            }
        }
        else if ((a == '+' || a == '-' || a == '*' || a == '/') && stack1.empty() && pred != 0) {
            stack1.push(a);
        }
        else if ((a == '+' || a == '-')) {
            if (pred == 0 || pred == '(' || pred == '[' || pred == '{') {
                stack1.push(a);
                B.push_back('0');
                B.push_back(' ');
                expressionOutput += "0";
            }
            else if (pred == '*' || pred == '/' || pred == '+' || pred == '-') {
                errorMessage = QString("ОШИБКА: Два оператора подряд: '%1%2'").arg(pred).arg(a);
                indicator = false;
            }
            else {
                while (!stack1.empty() && stack1.top() != '(' && stack1.top() != '[' && stack1.top() != '{') {
                    B.push_back(stack1.top());
                    B.push_back(' ');
                    stack1.pop();
                }
                stack1.push(a);
            }
        }
        else if ((a == '*' || a == '/')) {
            if (pred == 0 || pred == '(' || pred == '[' || pred == '{') {
                errorMessage = QString("ОШИБКА: Отсутствует левый операнд для '%1'").arg(a);
                indicator = false;
            }
            else if (pred == '*' || pred == '/' || pred == '+' || pred == '-') {
                errorMessage = QString("ОШИБКА: Два оператора подряд: '%1%2'").arg(pred).arg(a);
                indicator = false;
            }
            else {
                while (!stack1.empty() && stack1.top() != '(' && stack1.top() != '[' && stack1.top() != '{' &&
                       stack1.top() != '+' && stack1.top() != '-') {
                    B.push_back(stack1.top());
                    B.push_back(' ');
                    stack1.pop();
                }
                stack1.push(a);
            }
        }
        else {
            if (pred == ')' || pred == ']' || pred == '}') {
                errorMessage = QString("ОШИБКА: Отсутствует оператор после '%1' перед '%2'").arg(pred).arg(a);
                indicator = false;
            }
            else if (pred != 0 && pred != '(' && pred != ')' && pred != '[' && pred != ']' &&
                     pred != '{' && pred != '}' && pred != '+' && pred != '-' && pred != '*' && pred != '/') {
                // Для многосимвольных операндов удаляем пробел между символами
                if (!B.empty()) B.pop_back();
            }
            B.push_back(a);
            B.push_back(' ');
        }
        pred = a;
    }

    while (!stack1.empty() && indicator) {
        if (stack1.top() == '(') {
            errorMessage = "ОШИБКА: Незакрытая круглая скобка (";
            indicator = false;
        }
        else if (stack1.top() == '[') {
            errorMessage = "ОШИБКА: Незакрытая квадратная скобка [";
            indicator = false;
        }
        else if (stack1.top() == '{') {
            errorMessage = "ОШИБКА: Незакрытая фигурная скобка {";
            indicator = false;
        }
        else {
            B.push_back(stack1.top());
            B.push_back(' ');
            stack1.pop();
        }
    }

    if (!indicator) {
        appendToOutput("Выражение: " + expressionOutput, "blue");
        appendToOutput(errorMessage, "red");
        return false;
    }

    if (B.empty()) {
        appendToOutput("ОШИБКА: Пустое выражение после преобразования", "red");
        return false;
    }

    appendToOutput("Выражение успешно обработано", "green");
    return true;
}

bool MainWindow::calculateRPN(std::string &B, std::map<std::string, double> &operands)
{
    std::ifstream in(currentFile.toStdString());
    if (!in.is_open()) {
        appendToOutput("ERROR: ошибка при повторном открытии файла", "red");
        return false;
    }

    std::string line;
    std::getline(in, line);

    int lineNum = 2;
    while (std::getline(in, line)) {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

        if (line.empty())
            continue;

        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            appendToOutput(QString("ERROR: Строка %1 - пропуск '='").arg(lineNum),
                           "red");
            return false;
        }

        std::string name = line.substr(0, pos);
        size_t start = name.find_first_not_of(" \t");
        size_t end = name.find_last_not_of(" \t");
        if (start == std::string::npos) {
            appendToOutput(QString("ERROR: Строка %1 - отсутствует имя операнда").arg(lineNum), "red");
            return false;
        }
        name = name.substr(start, end - start + 1);

        if (std::all_of(name.begin(), name.end(), [](char c) { return std::isdigit(c); })) {
            appendToOutput(QString("ERROR: Строка %1 - имя операнда не может быть числом: '%2'")
                               .arg(lineNum)
                               .arg(QString::fromStdString(name)),
                               "red");
            return false;
        }

        std::string valueStr = line.substr(pos + 1);
        start = valueStr.find_first_not_of(" \t");
        if (start == std::string::npos) {
            appendToOutput(QString("ERROR: Строка %1 - пропущено значение операнда").arg(lineNum), "red");
            return false;
        }
        valueStr = valueStr.substr(start);

        std::replace(valueStr.begin(), valueStr.end(), ',', '.');

        try {
            double value = std::stod(valueStr);
            operands[name] = value;
            appendToOutput(QString("Операнд: %1 = %2").arg(QString::fromStdString(name)).arg(value),
                           "blue");
        } catch (const std::exception &) {
            appendToOutput(QString("ERROR: Строка %1 - некорректное значение: '%2'")
                               .arg(lineNum)
                               .arg(QString::fromStdString(valueStr)),
                               "red");
            return false;
        }

        lineNum++;
    }

    B += ' ';
    std::stack<double> stack2;
    std::string token;
    size_t pos = 0;
    bool indicator = true;
    QString calculationLog = "Шаги расчета:\n";

    while ((pos = B.find(' ')) != std::string::npos && indicator) {
        token = B.substr(0, pos);
        B.erase(0, pos + 1);

        if (token.empty())
            continue;

        if (token == "+" || token == "-" || token == "*" || token == "/") {
            if (stack2.size() < 2) {
                appendToOutput("ERROR: Недостаточно операндов для оператора: "
                                   + QString::fromStdString(token),
                               "red");
                return false;
            }

            double b = stack2.top();
            stack2.pop();
            double a = stack2.top();
            stack2.pop();
            double result = 0;

            if (token == "+")
                result = a + b;
            else if (token == "-")
                result = a - b;
            else if (token == "*")
                result = a * b;
            else if (token == "/") {
                if (b == 0) {
                    appendToOutput("ERROR: деление на ноль", "red");
                    return false;
                }
                result = a / b;
            }

            calculationLog += QString("\n  %1 %2 %3 = %4")
                                  .arg(a)
                                  .arg(QString::fromStdString(token))
                                  .arg(b)
                                  .arg(result);
            stack2.push(result);
        }
        else if (std::all_of(token.begin(), token.end(), [](char c) {
                     return std::isdigit(c) || c == '.' || c == ',';
                 })) {
            std::replace(token.begin(), token.end(), ',', '.');
            try {
                stack2.push(std::stod(token));
                calculationLog += QString("\n  Поместили операнд: %1").arg(QString::fromStdString(token));
            } catch (const std::exception &) {
                appendToOutput("ERROR: Некорректный числовой формат: " + QString::fromStdString(token), "red");
                return false;
            }
        }
        else {
            if (operands.find(token) == operands.end()) {
                appendToOutput("ERROR: Неопределённый операнд: " + QString::fromStdString(token), "red");
                return false;
            }
            double value = operands[token];
            stack2.push(value);
            calculationLog
                += QString("\n  Поместили операнд %1 = %2").arg(QString::fromStdString(token)).arg(value);
        }
    }

    if (stack2.size() != 1) {
        appendToOutput("ERROR: Неверно сформированное RPN выражение", "red");
        return false;
    }
    QString resultStr = formatDouble(stack2.top());

    appendToOutput(calculationLog, "green");
    appendToOutput("\nРезультат: " + resultStr, "white");
    return true;
}
QString MainWindow::formatDouble(double value) {
    QString result = QString::number(value, 'g', 10);
    result.replace('.', ',');

    if (result.contains(',') && result.split(',')[1].toDouble() == 0) {
        result = result.split(',')[0];
    }
    return result;
}
