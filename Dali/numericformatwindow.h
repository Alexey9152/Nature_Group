#ifndef NUMERICFORMATWINDOW_H
#define NUMERICFORMATWINDOW_H

#include <QMainWindow>
#include <QDateTime>
#include <QTimer>
#include <QCalendarWidget>
#include <QTimeEdit>

namespace Ui {
class NumericFormatWindow;
}

class NumericFormatWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum Mode {
        TimerMode,
        ClockMode,
    };
    explicit NumericFormatWindow(QWidget *parent = nullptr, Mode mode = ClockMode);
    ~NumericFormatWindow();

    QDateTime getCurrentDateTime() const;
    void setCustomDateTime(const QDateTime &dt);
    void setMode(Mode mode);

signals:

private slots:
    void updateTimeDisplay();
    void onCalendarDateChanged();
    void onTimeChanged(const QTime &time);
    void onSetToClockClicked();

private:
    void updateTextRepresentation();
    void setupTimeEdit();
    QString numberToWords(int number, bool ordinal = false, bool isFeminine = false) const;
    QString numberToOrdinalWords(int number) const;
    QString yearToWords(int year) const;
    QString monthToWords(int month) const;
    QString dayOfWeekToWords(int dayOfWeek) const;

    Ui::NumericFormatWindow *ui;
    QTimer *timer;
    QDateTime currentDateTime;
    bool isUsingSystemTimeForItself;
    Mode currentMode;
};
#endif
