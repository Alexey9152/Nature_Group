// manualinputwindow.h
#ifndef MANUALINPUTWINDOW_H
#define MANUALINPUTWINDOW_H

#include <QMainWindow>
#include <QDateTime>
#include <QMap>

namespace Ui {
class ManualInputWindow;
}

class ManualInputWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ManualInputWindow(QWidget *parent = nullptr);
    ~ManualInputWindow();

private slots:
    void on_inputDateTimeLineEdit_textChanged(const QString &text);

private:
    void updateVerbalRepresentation();

    QString numberToWords(int number, bool ordinal = false, bool isFeminine = false) const;
    QString numberToOrdinalWords(int number) const;
    QString yearToWords(int year) const;
    QString monthToWords(int month) const;
    QString dayOfWeekToWords(int dayOfWeek) const;
    QString monthToPrepositionalWords(int month) const;

    Ui::ManualInputWindow *ui;
    QDateTime currentParsedDateTime;
};

#endif // MANUALINPUTWINDOW_H
