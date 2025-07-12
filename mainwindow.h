#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <stack>
#include <queue>
#include <string>
#include <map>
#include <sstream>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <set>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void runProgram();
    void showAbout();
    void openFile();
    void clearOutput();
    void handleMissingVariable(const std::string& varName);

private:
    QTextEdit *textEdit;
    QPushButton *runButton;
    QPushButton *aboutButton;
    QPushButton *openButton;
    QPushButton *clearButton;

    QString participantName;
    QString currentFile;

    std::map<std::string, double> operands;
    std::string currentRPN;
    std::string currentOriginalExpression;
    std::set<std::string> variablesInExpression;

    // Вспомогательные функции
    void appendToOutput(const QString &text, const QString &color);
    bool checkBrackets(const std::string &expr, size_t &errorPos);
    bool isOperator(char c);
    bool checkOperators(const std::string &expr, size_t &errorPos);
    int getOperatorPriority(char op);
    bool convertToRPN(std::queue<char> &expressionQueue, std::string &RPN, size_t &errorLine, size_t &errorPos);
    bool calculateRPN(std::string &RPN, std::map<std::string, double> &operands);
    QString formatDouble(double value);
    bool readExpression(std::queue<char> &expressionQueue, std::string &originalExpression, size_t &errorLine, size_t &errorPos);
    void checkUnusedVariables();
};
#endif // MAINWINDOW_H
