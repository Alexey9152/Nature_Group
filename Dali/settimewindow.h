#ifndef SETTIMEWINDOW_H
#define SETTIMEWINDOW_H

#include <QDialog>
#include <QTimeEdit>
#include <QPushButton>
#include <QVBoxLayout>

class SetTimeWindow : public QDialog
{
    Q_OBJECT

public:
    explicit SetTimeWindow(QWidget *parent = nullptr);
    QTime getTime() const;

private:
    QTimeEdit *timeEdit;
};

#endif
