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

    // Create widgets
    textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    runButton = new QPushButton("Run Program", this);
    aboutButton = new QPushButton("About", this);
    openButton = new QPushButton("Open File", this);
    clearButton = new QPushButton("Clear Output", this);

    // Layout
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

    // Connections
    connect(runButton, &QPushButton::clicked, this, &MainWindow::runProgram);
    connect(aboutButton, &QPushButton::clicked, this, &MainWindow::showAbout);
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openFile);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearOutput);

    // Initial message
    appendToOutput("Expression Processor is ready", "blue");
    appendToOutput("Click 'Open File' to select input file or 'Run Program' to process the file");
}

void MainWindow::appendToOutput(const QString& text, const QString& color)
{
    textEdit->append(QString("<span style='color:%1;'>%2</span>").arg(color).arg(text));
}

void MainWindow::showAbout()
{
    QString aboutText =
        "THIS PROGRAM CHECKS FOR ERRORS WHEN WRITING EXPRESSIONS\n"
        "FROM A FILE, CONVERTS TO REVERSE POLISH NOTATION,\n"
        "CALCULATING THE VALUE OF THE EXPRESSION\n\n"
        "File format requirements:\n"
        "- First line: expression\n"
        "- Subsequent lines: operand definitions (name = value)\n"
        "- Operand names cannot be numbers and cannot contain '='\n\n"
        "Supported operations: +, -, *, /\n"
        "Supported brackets: (), [], {}";

    QMessageBox::information(this, "About Program", aboutText);
}

void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open Input File", "", "Text Files (*.txt)");
    if (!fileName.isEmpty()) {
        currentFile = fileName;
        appendToOutput("Selected file: " + fileName, "blue");
    }
}

void MainWindow::clearOutput()
{
    textEdit->clear();
}

void MainWindow::runProgram()
{
    if (currentFile.isEmpty()) {
        appendToOutput("ERROR: No file selected. Click 'Open File' first.", "red");
        return;
    }

    textEdit->clear();
    appendToOutput("Processing file: " + currentFile, "blue");

    std::queue<char> expression;
    if (!readExpression(expression)) {
        appendToOutput("Processing stopped due to errors", "red");
        return;
    }

    std::string RPN;
    if (!convertToRPN(expression, RPN)) {
        appendToOutput("Processing stopped due to errors", "red");
        return;
    }

    appendToOutput("Expression converted to RPN:", "green");
    appendToOutput(QString::fromStdString(RPN));

    std::map<std::string, double> operands;
    if (!calculateRPN(RPN, operands)) {
        appendToOutput("Processing stopped due to errors", "red");
    }
}

bool MainWindow::readExpression(std::queue<char>& expression)
{
    appendToOutput("\nReading expression from file...", "blue");

    std::ifstream in(currentFile.toStdString());
    if (!in.is_open()) {
        appendToOutput("ERROR: File did not open properly", "red");
        return false;
    }

    std::string line;
    if (!std::getline(in, line)) {
        appendToOutput("ERROR: File is empty", "red");
        return false;
    }

    // Remove carriage returns for Windows compatibility
    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

    for (char c : line) {
        expression.push(c);
    }

    appendToOutput("Expression read successfully:", "green");
    appendToOutput(QString::fromStdString(line));
    return true;
}

bool MainWindow::convertToRPN(std::queue<char>& expression, std::string& B)
{
    appendToOutput("\nConverting to RPN and checking for errors...", "blue");

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
                errorMessage = QString("ERROR: Operator absence before '%1'").arg(a);
                indicator = false;
            }
            else {
                stack1.push(a);
            }
        }
        else if ((a == '+' || a == '-' || a == '*' || a == '/') && expression.empty()) {
            errorMessage = QString("ERROR: Missing right operand after '%1'").arg(a);
            indicator = false;
        }
        else if (a == ')') {
            if (pred == '(') {
                errorMessage = "ERROR: Empty brackets ()";
                indicator = false;
            }
            else {
                if (stack1.empty()) {
                    errorMessage = "ERROR: Mismatched brackets - no opening bracket for )";
                    indicator = false;
                }
                else {
                    while (stack1.top() != '(' && indicator) {
                        if (stack1.empty()) {
                            errorMessage = "ERROR: Mismatched brackets - no opening bracket for )";
                            indicator = false;
                        }
                        else if (pred == '-' || pred == '+' || pred == '*' || pred == '/') {
                            errorMessage = "ERROR: Missing right operand";
                            indicator = false;
                        }
                        else if (stack1.top() == '[') {
                            errorMessage = "ERROR: Unclosed square bracket [";
                            indicator = false;
                        }
                        else if (stack1.top() == '{') {
                            errorMessage = "ERROR: Unclosed curly bracket {";
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
                errorMessage = "ERROR: Empty brackets []";
                indicator = false;
            }
            else {
                if (stack1.empty()) {
                    errorMessage = "ERROR: Mismatched brackets - no opening bracket for ]";
                    indicator = false;
                }
                else {
                    while (stack1.top() != '[' && indicator) {
                        if (stack1.empty()) {
                            errorMessage = "ERROR: Mismatched brackets - no opening bracket for ]";
                            indicator = false;
                        }
                        else if (pred == '-' || pred == '+' || pred == '*' || pred == '/') {
                            errorMessage = "ERROR: Missing right operand";
                            indicator = false;
                        }
                        else if (stack1.top() == '(') {
                            errorMessage = "ERROR: Unclosed round bracket (";
                            indicator = false;
                        }
                        else if (stack1.top() == '{') {
                            errorMessage = "ERROR: Unclosed curly bracket {";
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
                errorMessage = "ERROR: Empty brackets {}";
                indicator = false;
            }
            else {
                if (stack1.empty()) {
                    errorMessage = "ERROR: Mismatched brackets - no opening bracket for }";
                    indicator = false;
                }
                else {
                    while (stack1.top() != '{' && indicator) {
                        if (stack1.empty()) {
                            errorMessage = "ERROR: Mismatched brackets - no opening bracket for }";
                            indicator = false;
                        }
                        else if (pred == '-' || pred == '+' || pred == '*' || pred == '/') {
                            errorMessage = "ERROR: Missing right operand";
                            indicator = false;
                        }
                        else if (stack1.top() == '[') {
                            errorMessage = "ERROR: Unclosed square bracket [";
                            indicator = false;
                        }
                        else if (stack1.top() == '(') {
                            errorMessage = "ERROR: Unclosed round bracket (";
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
                errorMessage = QString("ERROR: Two operators in a row: '%1%2'").arg(pred).arg(a);
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
                errorMessage = QString("ERROR: Missing left operand for '%1'").arg(a);
                indicator = false;
            }
            else if (pred == '*' || pred == '/' || pred == '+' || pred == '-') {
                errorMessage = QString("ERROR: Two operators in a row: '%1%2'").arg(pred).arg(a);
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
                errorMessage = QString("ERROR: Operator absence after '%1' before '%2'").arg(pred).arg(a);
                indicator = false;
            }
            else if (pred != 0 && pred != '(' && pred != ')' && pred != '[' && pred != ']' &&
                     pred != '{' && pred != '}' && pred != '+' && pred != '-' && pred != '*' && pred != '/') {
                // For multi-character operands, remove space between characters
                if (!B.empty()) B.pop_back();
            }
            B.push_back(a);
            B.push_back(' ');
        }
        pred = a;
    }

    while (!stack1.empty() && indicator) {
        if (stack1.top() == '(') {
            errorMessage = "ERROR: Unclosed round bracket (";
            indicator = false;
        }
        else if (stack1.top() == '[') {
            errorMessage = "ERROR: Unclosed square bracket [";
            indicator = false;
        }
        else if (stack1.top() == '{') {
            errorMessage = "ERROR: Unclosed curly bracket {";
            indicator = false;
        }
        else {
            B.push_back(stack1.top());
            B.push_back(' ');
            stack1.pop();
        }
    }

    if (!indicator) {
        appendToOutput("Expression: " + expressionOutput, "black");
        appendToOutput(errorMessage, "red");
        return false;
    }

    if (B.empty()) {
        appendToOutput("ERROR: Empty expression after conversion", "red");
        return false;
    }

    appendToOutput("Expression processed successfully", "green");
    return true;
}

bool MainWindow::calculateRPN(std::string &B, std::map<std::string, double> &operands)
{
    appendToOutput("\nReading operands and calculating expression...", "blue");

    std::ifstream in(currentFile.toStdString());
    if (!in.is_open()) {
        appendToOutput("ERROR: Failed to reopen file for reading operands", "red");
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
            appendToOutput(QString("ERROR: Line %1 - missing '=' in operand definition").arg(lineNum),
                           "red");
            return false;
        }

        std::string name = line.substr(0, pos);
        size_t start = name.find_first_not_of(" \t");
        size_t end = name.find_last_not_of(" \t");
        if (start == std::string::npos) {
            appendToOutput(QString("ERROR: Line %1 - missing operand name").arg(lineNum), "red");
            return false;
        }
        name = name.substr(start, end - start + 1);

        if (std::all_of(name.begin(), name.end(), [](char c) { return std::isdigit(c); })) {
            appendToOutput(QString("ERROR: Line %1 - operand name cannot be number: '%2'")
                               .arg(lineNum)
                               .arg(QString::fromStdString(name)),
                               "red");
            return false;
        }

        std::string valueStr = line.substr(pos + 1);
        start = valueStr.find_first_not_of(" \t");
        if (start == std::string::npos) {
            appendToOutput(QString("ERROR: Line %1 - missing operand value").arg(lineNum), "red");
            return false;
        }
        valueStr = valueStr.substr(start);

        std::replace(valueStr.begin(), valueStr.end(), ',', '.');

        try {
            double value = std::stod(valueStr);
            operands[name] = value;
            appendToOutput(QString("Operand: %1 = %2").arg(QString::fromStdString(name)).arg(value),
                           "darkblue");
        } catch (const std::exception &) {
            appendToOutput(QString("ERROR: Line %1 - invalid operand value: '%2'")
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
    QString calculationLog = "Calculation steps:";

    while ((pos = B.find(' ')) != std::string::npos && indicator) {
        token = B.substr(0, pos);
        B.erase(0, pos + 1);

        if (token.empty())
            continue;

        if (token == "+" || token == "-" || token == "*" || token == "/") {
            if (stack2.size() < 2) {
                appendToOutput("ERROR: Insufficient operands for operator: "
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
                    appendToOutput("ERROR: Division by zero", "red");
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
                calculationLog += QString("\n  Push number: %1").arg(QString::fromStdString(token));
            } catch (const std::exception &) {
                appendToOutput("ERROR: Invalid number format: " + QString::fromStdString(token), "red");
                return false;
            }
        }
        else {
            if (operands.find(token) == operands.end()) {
                appendToOutput("ERROR: Undefined operand: " + QString::fromStdString(token), "red");
                return false;
            }
            double value = operands[token];
            stack2.push(value);
            calculationLog
                += QString("\n  Push operand %1 = %2").arg(QString::fromStdString(token)).arg(value);
        }
    }

    if (stack2.size() != 1) {
        appendToOutput("ERROR: Malformed RPN expression", "red");
        return false;
    }
    QString resultStr = formatDouble(stack2.top());

    appendToOutput(calculationLog, "darkgreen");
    appendToOutput("\nResult: " + resultStr, "green");
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
