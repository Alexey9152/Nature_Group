#include "mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <cmath>
#include <QLocale>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>

    MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), participantName("NatureGroupVUS") {
    setWindowTitle(participantName);
    resize(800, 600);

    textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    runButton = new QPushButton("Запустить", this);
    aboutButton = new QPushButton("О программе", this);
    openButton = new QPushButton("Открыть файл", this);
    clearButton = new QPushButton("Очистить вывод", this);

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

    appendToOutput("Готов к обработке выражений. Загрузите файл с выражением.", "black");
}

void MainWindow::appendToOutput(const QString &text, const QString &color) {
    textEdit->append(QString("<span style='color:%1;'>%2</span>").arg(color).arg(text));
}

void MainWindow::showAbout() {
    QString aboutText = "Эта программа проверяет синтаксис выражений, преобразует их в ОПЗ и вычисляет результат.\n"
                        "Поддерживаемые операции: +, -, *, /\n"
                        "Поддерживаемые скобки: (), [], {}\n"
                        "Переменные могут быть определены в файле или введены вручную (используйте '.' или ',' в качестве десятичного разделителя).\n"
                        "В ОПЗ унарный минут обозначается ~, унарный плюс обозначается #";
    QMessageBox::information(this, "О программе", aboutText);
}

void MainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Открыть файл", "", "Текстовые файлы (*.txt);;Все файлы (*)");
    if (!fileName.isEmpty()) {
        currentFile = fileName;
        appendToOutput("Открыт файл: " + fileName, "blue");
        operands.clear();
        variablesInExpression.clear();
    }
}

void MainWindow::clearOutput() {
    textEdit->clear();
    operands.clear();
    currentRPN.clear();
    currentOriginalExpression.clear();
    variablesInExpression.clear();
}

bool MainWindow::checkBrackets(const std::string &expr, size_t &errorPos) {
    std::stack<char> brackets;
    for (size_t i = 0; i < expr.size(); ++i) {
        char c = expr[i];
        if (c == '(' || c == '[' || c == '{') {
            brackets.push(c);
        } else if (c == ')' || c == ']' || c == '}') {
            if (brackets.empty()) {
                errorPos = i;
                return false;
            }
            char top = brackets.top();
            if ((c == ')' && top != '(') || (c == ']' && top != '[') || (c == '}' && top != '{')) {
                errorPos = i;
                return false;
            }
            brackets.pop();
        }
    }
    if (!brackets.empty()) {
        errorPos = expr.size();
        return false;
    }
    return true;
}

bool MainWindow::isOperator(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '/');
}

bool MainWindow::checkOperators(const std::string &expr, size_t &errorPos) {
    bool lastWasOperator = true;
    bool lastWasOperandOrClosingBracket = false;

    for (size_t i = 0; i < expr.size(); ++i) {
        char c = expr[i];

        if (isspace(c)) {
            continue;
        }

        if (isdigit(c) || isalpha(c) || (c == '.')) {
            if (i > 0) {
                char prev_char = expr[i-1];
                if ((isdigit(c) && (isdigit(prev_char) || prev_char == '.')) ||
                    (isalpha(c) && (isalpha(prev_char) || isdigit(prev_char))) ||
                    (c == '.' && isdigit(prev_char))
                    )
                {
                    continue;
                }
            }

            if (lastWasOperandOrClosingBracket && c != '.') {
                errorPos = i;
                appendToOutput(QString("ОШИБКА: Отсутствует оператор перед '%1' на позиции %2. (Возможно неявное умножение?)").arg(c).arg(i+1), "red");
                return false;
            }

            if (c == '.' && (i == 0 || !isdigit(expr[i-1]))) {
                size_t next_char_idx = i + 1;
                while (next_char_idx < expr.size() && isspace(expr[next_char_idx])) {
                    next_char_idx++;
                }
                if (next_char_idx >= expr.size() || !isdigit(expr[next_char_idx])) {
                    errorPos = i;
                    appendToOutput(QString("ОШИБКА: Некорректное использование десятичной точки на позиции %1. Ожидалась цифра после точки.").arg(i+1), "red");
                    return false;
                }
            }

            lastWasOperator = false;
            lastWasOperandOrClosingBracket = true;
        } else if (isOperator(c)) {
            bool isUnary = false;
            if (lastWasOperator || i == 0) {
                if (c == '+' || c == '-') {
                    isUnary = true;
                }
            }

            if (!isUnary && lastWasOperator) {
                errorPos = i;
                appendToOutput(QString("ОШИБКА: Неверная последовательность операторов на позиции %1.").arg(i+1), "red");
                return false;
            } else if (isUnary && (c == '*' || c == '/')) {
                errorPos = i;
                appendToOutput(QString("ОШИБКА: Унарный оператор '%1' не разрешен на позиции %2.").arg(c).arg(i+1), "red");
                return false;
            }

            lastWasOperator = true;
            lastWasOperandOrClosingBracket = false;
        } else if (c == '(' || c == '[' || c == '{') {
            if (lastWasOperandOrClosingBracket) {
                errorPos = i;
                appendToOutput(QString("ОШИБКА: Отсутствует оператор перед открывающейся скобкой на позиции %1.").arg(i+1), "red");
                return false;
            }
            lastWasOperator = true;
            lastWasOperandOrClosingBracket = false;
        } else if (c == ')' || c == ']' || c == '}') {
            if (lastWasOperator) {
                errorPos = i;
                appendToOutput(QString("ОШИБКА: Оператор непосредственно перед закрывающей скобкой на позиции %1.").arg(i+1), "red");
                return false;
            }
            lastWasOperator = false;
            lastWasOperandOrClosingBracket = true;
        } else {
            errorPos = i;
            appendToOutput(QString("ОШИБКА: Неожиданный символ '%1' на позиции %2.").arg(c).arg(i+1), "red");
            return false;
        }
    }

    if (lastWasOperator && expr.size() > 0 && !(expr.back() == ')' || expr.back() == ']' || expr.back() == '}')) {
        errorPos = expr.size() - 1;
        appendToOutput(QString("ОШИБКА: Выражение заканчивается оператором на позиции %1.").arg(errorPos+1), "red");
        return false;
    }
    return true;
}

int MainWindow::getOperatorPriority(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    if (op == '~' || op == '#') return 3;
    return 0;
}

bool MainWindow::convertToRPN(std::queue<char> &expressionQueue, std::string &RPN, size_t &errorLine, size_t &errorPos) {
    std::stack<char> operators;
    std::string currentToken;
    RPN.clear();
    variablesInExpression.clear();
    bool expectOperand = true;

    std::queue<char> tempQueue = expressionQueue;

    size_t original_pos = 0;

    while (!tempQueue.empty()) {
        char c = tempQueue.front();
        tempQueue.pop();

        if (isspace(c)) {
            original_pos++;
            continue;
        }

        if (isdigit(c) || isalpha(c) || c == '.') {
            currentToken += c;
            expectOperand = false;
        } else {
            if (!currentToken.empty()) {
                if (isalpha(currentToken[0])) {
                    variablesInExpression.insert(currentToken);
                }
                RPN += currentToken + " ";
                currentToken.clear();
            }

            if (c == '(' || c == '[' || c == '{') {
                operators.push(c);
                expectOperand = true;
            } else if (c == ')' || c == ']' || c == '}') {
                char openingBracket = '\0';
                if (c == ')') openingBracket = '(';
                else if (c == ']') openingBracket = '[';
                else if (c == '}') openingBracket = '{';

                while (!operators.empty() && operators.top() != openingBracket) {
                    RPN += std::string(1, operators.top()) + " ";
                    operators.pop();
                }
                if (operators.empty()) {
                    appendToOutput("ОШИБКА: Несоответствующая закрывающая скобка во время конвертации RPN. Это указывает на серьезную синтаксическую проблему.", "red");
                    errorPos = original_pos;
                    return false;
                }
                operators.pop();
                expectOperand = false;
            } else if (isOperator(c)) {
                if (expectOperand) {
                    if (c == '-') {
                        operators.push('~');
                    } else if (c == '+') {
                        operators.push('#');
                    } else {
                        appendToOutput(QString("ОШИБКА: Недопустимый унарный оператор '%1' на позиции %2 в выражении.").arg(c).arg(original_pos + 1), "red");
                        return false;
                    }
                    expectOperand = true;
                } else {
                    while (!operators.empty() &&
                           getOperatorPriority(operators.top()) >= getOperatorPriority(c)) {
                        RPN += std::string(1, operators.top()) + " ";
                        operators.pop();
                    }
                    operators.push(c);
                    expectOperand = true;
                }
            } else {
                appendToOutput(QString("ОШИБКА: Неожиданный символ '%1' во время конвертации RPN на позиции %2. Это может указывать на проблему с токенизацией или необработанный символ.").arg(c).arg(original_pos + 1), "red");
                return false;
            }
        }
        original_pos++;
    }

    if (!currentToken.empty()) {
        if (isalpha(currentToken[0])) {
            variablesInExpression.insert(currentToken);
        }
        RPN += currentToken + " ";
        currentToken.clear();
    }

    while (!operators.empty()) {
        if (operators.top() == '(' || operators.top() == '[' || operators.top() == '{') {
            appendToOutput("ОШИБКА: Несоответствующая открывающая скобка осталась после конвертации RPN. Это указывает на дисбаланс скобок.", "red");
            errorPos = original_pos;
            return false;
        }
        RPN += std::string(1, operators.top()) + " ";
        operators.pop();
    }

    appendToOutput("RPN (Обратная польская запись): " + QString::fromStdString(RPN), "darkBlue");
    return true;
}

bool MainWindow::calculateRPN(std::string &RPN, std::map<std::string, double> &operands)
{
    std::stack<double> values;
    std::istringstream iss(RPN);
    std::string token;

    while (iss >> token) {
        if (token == "+" || token == "-" || token == "*" || token == "/") {
            if (values.size() < 2) {
                appendToOutput("ОШИБКА: Недостаточно операндов для оператора '" + QString::fromStdString(token) + "' в RPN.", "red");
                return false;
            }
            double b = values.top(); values.pop();
            double a = values.top(); values.pop();
            double result = 0;
            if (token == "+") result = a + b;
            else if (token == "-") result = a - b;
            else if (token == "*") result = a * b;
            else if (token == "/") {
                if (b == 0) {
                    appendToOutput("ОШИБКА: Обнаружено деление на ноль.", "red");
                    return false;
                }
                result = a / b;
            }
            values.push(result);
        } else if (token == "~" || token == "#") {
            if (values.empty()) {
                appendToOutput("ОШИБКА: Недостаточно операндов для унарного оператора '" + QString::fromStdString(token) + "' в RPN.", "red");
                return false;
            }
            double val = values.top(); values.pop();
            if (token == "~") {
                values.push(-val);
            } else {
                values.push(val);
            }
        }
        else {
            bool isNumber = false;
            std::istringstream numStream(token);
            numStream.imbue(std::locale::classic());
            double numVal;
            numStream >> numVal;
            if (!numStream.fail() && numStream.eof()) {
                isNumber = true;
            }

            if (isNumber) {
                values.push(numVal);
            } else {
                if (operands.find(token) == operands.end()) {
                    appendToOutput(QString("ОШИБКА: Неопределенная переменная '%1'. Запрос значения...").arg(QString::fromStdString(token)), "red");
                    currentRPN = RPN;
                    QMetaObject::invokeMethod(this, "handleMissingVariable", Qt::QueuedConnection,
                                              Q_ARG(std::string, token));
                    return false;
                }
                values.push(operands[token]);
            }
        }
    }

    if (values.size() != 1) {
        appendToOutput("ОШИБКА: Неверное RPN выражение или несбалансированные операнды/операторы (результатом должно быть одно значение).", "red");
        return false;
    }

    appendToOutput("Результат вычисления: " + formatDouble(values.top()), "darkBlue");
    return true;
}

QString MainWindow::formatDouble(double value) {
    QString str = QString::number(value, 'f', 6);
    while (str.endsWith('0') && str.contains('.')) {
        str.chop(1);
    }
    if (str.endsWith('.')) {
        str.chop(1);
    }
    return str;
}


void MainWindow::runProgram() {
    if (currentFile.isEmpty()) {
        appendToOutput("ОШИБКА: Файл не открыт. Пожалуйста, сначала откройте файл.", "red");
        return;
    }

    textEdit->clear();

    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString formattedDateTime = currentDateTime.toString("yyyy-MM-dd hh:mm:ss");
    appendToOutput(QString("--- Программа запущена: %1 ---").arg(formattedDateTime), "darkcyan");

    appendToOutput("Обрабатывается файл: " + currentFile, "black");

    QFile displayFile(currentFile);
    if (!displayFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        appendToOutput("ОШИБКА: Не удалось открыть файл для отображения содержимого: " + currentFile, "red");
        return;
    }
    appendToOutput("\n--- Содержимое файла ---", "darkcyan");
    QTextStream displayStream(&displayFile);
    while (!displayStream.atEnd()) {
        appendToOutput(displayStream.readLine(), "black");
    }
    displayFile.close();
    appendToOutput("--------------------", "darkcyan");

    std::queue<char> expressionQueue;
    size_t errorLine = 0, errorPos = 0;

    operands.clear();
    variablesInExpression.clear();
    if (!readExpression(expressionQueue, currentOriginalExpression, errorLine, errorPos)) {
        appendToOutput("Пожалуйста, исправьте входной файл.", "red");
        return;
    }

    std::string newRPN;
    bool rpnConverted = convertToRPN(expressionQueue, newRPN, errorLine, errorPos);

    if (!rpnConverted) {
        appendToOutput("Ошибка преобразования в RPN из-за синтаксических ошибок выражения.", "red");
        return;
    }
    currentRPN = newRPN;
    if (currentRPN.empty() && currentOriginalExpression.empty()) {
        appendToOutput("В файле не найдено выражение для вычисления.", "orange");
        return;
    }

    if (!calculateRPN(currentRPN, operands)) {
        appendToOutput("Ошибка вычисления выражения.", "red");
    } else {
        appendToOutput("--- Выполнение программы успешно завершено ---", "darkcyan");
    }

    checkUnusedVariables();
}

void MainWindow::handleMissingVariable(const std::string& varName)
{
    bool ok;
    QString valueStr = QInputDialog::getText(this, "Ввод отсутствующей переменной",
                                             QString("Введите значение для переменной '%1':").arg(QString::fromStdString(varName)) + "\n(Используйте '.' или ',' для десятичных знаков)",
                                             QLineEdit::Normal, "", &ok);
    if (ok && !valueStr.isEmpty()) {
        if (valueStr.contains(',')) {
            valueStr.replace(',', '.');
        }

        bool conversionOk;
        double value = valueStr.toDouble(&conversionOk);
        if (conversionOk) {
            operands[varName] = value;
            appendToOutput(QString("Определена переменная: %1 = %2").arg(QString::fromStdString(varName)).arg(formatDouble(value)), "green");
            if (!calculateRPN(currentRPN, operands)) {
                appendToOutput("Повторная попытка вычисления с новой переменной завершилась ошибкой.", "red");
            } else {
                appendToOutput("--- Выполнение программы успешно завершено ---", "darkgreen");
            }
        } else {
            appendToOutput("ОШИБКА: Введен неверный формат числа для переменной. Убедитесь, что используется только один десятичный разделитель (точка или запятая).", "red");
            QMetaObject::invokeMethod(this, "handleMissingVariable", Qt::QueuedConnection,
                                      Q_ARG(std::string, varName));
        }
    } else {
        appendToOutput(QString("Переменная '%1' не определена. Вычисление прервано.").arg(QString::fromStdString(varName)), "orange");
    }
}

void MainWindow::checkUnusedVariables() {
    for (const auto& pair : operands) {
        const std::string& varName = pair.first;
        if (variablesInExpression.find(varName) == variablesInExpression.end()) {
            appendToOutput(QString("ПРЕДУПРЕЖДЕНИЕ: Переменная '%1' определена, но не используется в выражении.").arg(QString::fromStdString(varName)), "orange");
        }
    }
}


bool MainWindow::readExpression(std::queue<char> &expressionQueue, std::string &originalExpression, size_t &errorLine, size_t &errorPos) {
    QFile file(currentFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        appendToOutput("ОШИБКА: Не удалось открыть файл: " + currentFile, "red");
        return false;
    }

    QTextStream in(&file);
    QString line;
    originalExpression.clear();
    expressionQueue = std::queue<char>();

    bool expressionFound = false;
    int currentLineNum = 0;

    std::map<std::string, double> tempOperandsFromFile;

    while (!in.atEnd()) {
        line = in.readLine();
        currentLineNum++;

        if (line.trimmed().isEmpty()) {
            continue;
        }

        if (line.contains('=')) {
            QString trimmedLine = line.trimmed();
            int eqPos = trimmedLine.indexOf('=');
            if (eqPos == -1) {
                errorLine = currentLineNum;
                appendToOutput(QString("ОШИБКА: Неверный формат строки переменной на строке %1: %2").arg(currentLineNum).arg(line), "red");
                return false;
            }

            QString varName = trimmedLine.left(eqPos).trimmed();
            QString varValueStr = trimmedLine.mid(eqPos + 1).trimmed();

            if (varValueStr.contains(',')) {
                varValueStr.replace(',', '.');
            }

            bool ok;
            double varValue = varValueStr.toDouble(&ok);
            if (!ok) {
                errorLine = currentLineNum;
                appendToOutput(QString("ОШИБКА: Неверный формат числа для переменной '%1' на строке %2: %3").arg(varName).arg(currentLineNum).arg(varValueStr), "red");
                return false;
            }

            tempOperandsFromFile[varName.toStdString()] = varValue;
            //appendToOutput(QString("Переменная из файла: %1 = %2").arg(varName).arg(formatDouble(varValue)), "green");
        } else {
            if (expressionFound) {
                appendToOutput(QString("Предупреждение: Пропущена дополнительная строка '%1' после выражения. Обрабатывается только первое выражение.").arg(line), "orange");
                continue;
            }
            originalExpression = line.toStdString();
            expressionFound = true;

            size_t tempErrorPos = 0;
            if (!checkBrackets(originalExpression, tempErrorPos)) {
                errorLine = currentLineNum;
                errorPos = tempErrorPos;
                appendToOutput(QString("ОШИБКА: Несбалансированные скобки на строке %1, позиция %2.").arg(currentLineNum).arg(tempErrorPos + 1), "red");
                return false;
            }
            if (!checkOperators(originalExpression, tempErrorPos)) {
                errorLine = currentLineNum;
                errorPos = tempErrorPos;
                return false;
            }

            for (char c : originalExpression) {
                if (!isspace(c)) {
                    expressionQueue.push(c);
                }
            }
        }
    }

    file.close();

    if (originalExpression.empty()) {
        appendToOutput("ОШИБКА: В файле не найдено выражение для вычисления.", "red");
        return false;
    }

    operands.insert(tempOperandsFromFile.begin(), tempOperandsFromFile.end());

    return true;
}
