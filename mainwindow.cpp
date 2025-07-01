#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Проверка скобочных последовательностей");
    setFixedSize(400, 200);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    inputField = new QLineEdit(this);
    inputField->setPlaceholderText("Введите скобочную последовательность");

    checkButton = new QPushButton("Проверить", this);

    resultLabel = new QLabel("Результат: ожидание ввода", this);
    resultLabel->setAlignment(Qt::AlignCenter);

    layout->addWidget(inputField);
    layout->addWidget(checkButton);
    layout->addWidget(resultLabel);

    setCentralWidget(centralWidget);

    connect(checkButton, &QPushButton::clicked, this, &MainWindow::checkSequence);
    connect(inputField, &QLineEdit::returnPressed, this, &MainWindow::checkSequence);
}

MainWindow::~MainWindow() {}

bool MainWindow::isValidSequence(const QString& sequence)
{
    int balance = 0;

    for (const QChar& ch : sequence) {
        if (ch == '(') {
            balance++;
        } else if (ch == ')') {
            balance--;
            if (balance < 0) return false;
        }
    }

    return balance == 0;
}

void MainWindow::checkSequence()
{
    QString text = inputField->text();

    if (text.isEmpty()) {
        resultLabel->setText("Результат: введите последовательность");
        return;
    }

    if (isValidSequence(text)) {
        resultLabel->setText("Результат: ✔ Правильная последовательность");
        resultLabel->setStyleSheet("color: green; font-weight: bold;");
    } else {
        resultLabel->setText("Результат: ❌ Неправильная последовательность");
        resultLabel->setStyleSheet("color: red; font-weight: bold;");
    }
}
