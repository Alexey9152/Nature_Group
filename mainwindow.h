#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFile>
#include <QMainWindow>
#include <QPushButton>
#include <QTextEdit>
#include <map>
#include <queue>
#include <stack>
#include <string>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void runProgram();
    void showAbout();
    void openFile();
    void clearOutput();

private:
    bool readExpression(std::queue<char> &expression);
    bool convertToRPN(std::queue<char> &expression, std::string &B);
    bool calculateRPN(std::string &B, std::map<std::string, double> &operands);
    void appendToOutput(const QString &text, const QString &color = "black");
    QString formatDouble(double value);
    bool convertToBinaryAndSave(const QString& txtFileName, const QString& binFileName);

    QTextEdit *textEdit;
    QPushButton *runButton;
    QPushButton *aboutButton;
    QPushButton *openButton;
    QPushButton *clearButton;
    QString currentFile;
};

#endif // MAINWINDOW_H
