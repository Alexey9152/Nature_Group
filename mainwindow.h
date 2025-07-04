#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QTcpSocket>
#include <QFileDialog>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <fstream>
#include <queue>
#include <map>
#include <algorithm>
#include <cctype>

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
    void readServerResponse();

private:
    bool readExpression(std::queue<char> &expression);
    void sendToServer(const QString& expression, const std::map<std::string, double>& operands);
    QString formatDouble(double value);
    void appendToOutput(const QString &text, const QString &color = "black");

    QTextEdit *textEdit;
    QPushButton *runButton;
    QPushButton *aboutButton;
    QPushButton *openButton;
    QPushButton *clearButton;
    QLineEdit *ipEdit;       // Поле для ввода IP
    QSpinBox *portSpin;      // Поле для ввода порта
    QTcpSocket *tcpSocket;
    QString currentFile;
};

#endif // MAINWINDOW_H
