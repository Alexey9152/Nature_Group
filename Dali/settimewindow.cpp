#include "settimewindow.h"

SetTimeWindow::SetTimeWindow(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Установить время");
    setModal(true);
    setFixedSize(200, 100);

    timeEdit = new QTimeEdit(this);
    timeEdit->setDisplayFormat("HH:mm:ss");
    timeEdit->setTime(QTime::currentTime());

    QPushButton *okButton = new QPushButton("OK", this);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(timeEdit);
    layout->addWidget(okButton);
    setLayout(layout);
}

QTime SetTimeWindow::getTime() const
{
    return timeEdit->time();
}
