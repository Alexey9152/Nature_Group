#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void checkSequence();

private:
    QLineEdit *inputField;
    QPushButton *checkButton;
    QLabel *resultLabel;

    bool isValidSequence(const QString& sequence);
};

#endif // MAINWINDOW_H
