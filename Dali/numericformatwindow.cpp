#include "numericformatwindow.h"
#include "ui_numericformatwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <QMap>

NumericFormatWindow::NumericFormatWindow(QWidget *parent, Mode mode)
    : QMainWindow(parent)
    , ui(new Ui::NumericFormatWindow)
    , isUsingSystemTimeForItself(true)
    , currentMode(mode)
{
    ui->setupUi(this);
    setWindowTitle("Числовой формат");

    currentDateTime = QDateTime::currentDateTime();
    ui->calendarWidget->setSelectedDate(currentDateTime.date());
    setupTimeEdit();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &NumericFormatWindow::updateTimeDisplay);

    timer->start(1000);

    connect(ui->calendarWidget, &QCalendarWidget::selectionChanged,
            this, &NumericFormatWindow::onCalendarDateChanged);
    connect(ui->timeEdit, &QTimeEdit::timeChanged,
            this, &NumericFormatWindow::onTimeChanged);

    setMode(mode);
    updateTextRepresentation();
}

void NumericFormatWindow::setupTimeEdit()
{
    ui->timeEdit->setDisplayFormat("HH:mm:ss");
    ui->timeEdit->setTime(currentDateTime.time());
}

void NumericFormatWindow::setMode(Mode mode)
{
    currentMode = mode;
    if (currentMode == ClockMode) {
        isUsingSystemTimeForItself = true;
        currentDateTime = QDateTime::currentDateTime();
        ui->calendarWidget->setSelectedDate(currentDateTime.date());
        ui->timeEdit->setTime(currentDateTime.time());
    } else {
        isUsingSystemTimeForItself = false;
    }
    updateTextRepresentation();
}

QDateTime NumericFormatWindow::getCurrentDateTime() const
{
    return currentDateTime;
}

void NumericFormatWindow::setCustomDateTime(const QDateTime &dt)
{
    currentDateTime = dt;
    isUsingSystemTimeForItself = false;
    ui->calendarWidget->setSelectedDate(currentDateTime.date());
    ui->timeEdit->setTime(currentDateTime.time());
    updateTextRepresentation();
}

void NumericFormatWindow::updateTimeDisplay()
{
    if (currentMode == ClockMode) {
        if (isUsingSystemTimeForItself) {
            currentDateTime = QDateTime::currentDateTime();
            ui->timeEdit->setTime(currentDateTime.time());
            ui->calendarWidget->setSelectedDate(currentDateTime.date());
        }
    } else if (currentMode == TimerMode) {
        currentDateTime = currentDateTime.addSecs(1);
        ui->timeEdit->setTime(currentDateTime.time());
    }

    updateTextRepresentation();
}

void NumericFormatWindow::onTimeChanged(const QTime &time)
{
    isUsingSystemTimeForItself = false;
    currentDateTime.setTime(time);
    updateTextRepresentation();
}

void NumericFormatWindow::onCalendarDateChanged()
{
    isUsingSystemTimeForItself = false;
    currentDateTime.setDate(ui->calendarWidget->selectedDate());
    updateTextRepresentation();
}

void NumericFormatWindow::onSetToClockClicked()
{
    isUsingSystemTimeForItself = true;
    currentDateTime = QDateTime::currentDateTime();
    ui->calendarWidget->setSelectedDate(currentDateTime.date());
    ui->timeEdit->setTime(currentDateTime.time());
    updateTextRepresentation();
}

QString NumericFormatWindow::numberToWords(int number, bool ordinal, bool isFeminine) const
{
    if (number == 0) {
        if (ordinal) return "нулевого";
        return "ноль";
    }
    if (number < 0 || number > 9999) return QString::number(number);

    QString result;

    static const QMap<int, QString> units = {
        {1, "один"}, {2, "два"}, {3, "три"}, {4, "четыре"}, {5, "пять"},
        {6, "шесть"}, {7, "семь"}, {8, "восемь"}, {9, "девять"}
    };
    static const QMap<int, QString> unitsFeminine = {
        {1, "одна"}, {2, "две"}
    };
    static const QMap<int, QString> teens = {
        {10, "десять"}, {11, "одиннадцать"}, {12, "двенадцать"}, {13, "тринадцать"},
        {14, "четырнадцать"}, {15, "пятнадцать"}, {16, "шестнадцать"},
        {17, "семнадцать"}, {18, "восемнадцать"}, {19, "девятнадцать"}
    };
    static const QMap<int, QString> tens = {
        {20, "двадцать"}, {30, "тридцать"}, {40, "сорок"}, {50, "пятьдесят"},
        {60, "шестьдесят"}, {70, "семьдесят"}, {80, "восемьдесят"}, {90, "девяносто"}
    };
    static const QMap<int, QString> hundreds = {
        {100, "сто"}, {200, "двести"}, {300, "триста"}, {400, "четыреста"},
        {500, "пятьсот"}, {600, "шестьсот"}, {700, "семьсот"},
        {800, "восемьсот"}, {900, "девятьсот"}
    };

    static const QMap<int, QString> ordinalUnits = {
        {1, "первого"}, {2, "второго"}, {3, "третьего"}, {4, "четвертого"}, {5, "пятого"},
        {6, "шестого"}, {7, "седьмого"}, {8, "восьмого"}, {9, "девятого"}
    };
    static const QMap<int, QString> ordinalTeens = {
        {10, "десятого"}, {11, "одиннадцатого"}, {12, "двенадцатого"}, {13, "тринадцатого"},
        {14, "четырнадцатого"}, {15, "пятнадцатого"}, {16, "шестнадцатого"},
        {17, "семнадцатого"}, {18, "восемнадцатого"}, {19, "девятнадцатого"}
    };
    static const QMap<int, QString> ordinalTens = {
        {20, "двадцатого"}, {30, "тридцатого"}, {40, "сорокового"}, {50, "пятидесятого"},
        {60, "шестидесятого"}, {70, "семидесятого"}, {80, "восьмидесятого"}, {90, "девяностого"}
    };
    static const QMap<int, QString> ordinalHundreds = {
        {100, "сотого"}, {200, "двухсотого"}, {300, "трехсотого"}, {400, "четырехсотого"},
        {500, "пятисотого"}, {600, "шестисотого"}, {700, "семисотого"}, {800, "восьмисотого"}, {900, "девятисотого"}
    };

    if (number >= 100) {
        int hundredsPart = (number / 100) * 100;
        int remainder = number % 100;

        if (ordinal) {
            result += ordinalHundreds.value(hundredsPart, "");
            if (remainder > 0) {
                result += " " + numberToOrdinalWords(remainder);
            }
        } else {
            result += hundreds.value(hundredsPart, "");
            if (remainder > 0) {
                result += " " + numberToWords(remainder, false, isFeminine);
            }
        }
    } else if (number >= 20) {
        if (ordinal) {
            if (number % 10 == 0) {
                result += ordinalTens.value(number, "");
            } else {
                result += tens.value(number - (number % 10), "");
                result += " " + ordinalUnits.value(number % 10, "");
            }
        } else {
            result += tens.value(number - (number % 10), "");
            if (number % 10 != 0) {
                if (isFeminine) {
                    if (number % 10 == 1) result += " " + unitsFeminine.value(1, "");
                    else if (number % 10 == 2) result += " " + unitsFeminine.value(2, "");
                    else result += " " + units.value(number % 10, "");
                } else {
                    result += " " + units.value(number % 10, "");
                }
            }
        }
    } else if (number >= 10) {
        if (ordinal) {
            result += ordinalTeens.value(number, "");
        } else {
            result += teens.value(number, "");
        }
    } else {
        if (ordinal) {
            result += ordinalUnits.value(number, "");
        } else {
            if (isFeminine) {
                result += unitsFeminine.value(number, units.value(number, ""));
            } else {
                result += units.value(number, "");
            }
        }
    }

    return result.trimmed();
}

QString NumericFormatWindow::numberToOrdinalWords(int number) const {
    return numberToWords(number, true);
}

QString NumericFormatWindow::yearToWords(int year) const {
    QString result;
    if (year < 1000) {
        return numberToOrdinalWords(year);
    } else {
        int thousands = year / 1000;
        int remainder = year % 1000;

        if (thousands == 1) result += "тысяча";
        else if (thousands == 2) result += "две тысячи";
        else if (thousands >= 5 && thousands <= 20) result += numberToWords(thousands) + " тысяч";
        else if (thousands % 10 == 1 && thousands != 11) result += numberToWords(thousands) + " тысяча";
        else if (thousands % 10 >= 2 && thousands % 10 <= 4 && (thousands < 10 || thousands > 20)) result += numberToWords(thousands) + " тысячи";
        else result += numberToWords(thousands) + " тысяч";


        if (remainder > 0) {
            result += " " + numberToOrdinalWords(remainder);
        }
    }
    return result.trimmed();
}

QString NumericFormatWindow::monthToWords(int month) const
{
    static const QMap<int, QString> months = {
        {1, "января"}, {2, "февраля"}, {3, "марта"}, {4, "апреля"}, {5, "мая"}, {6, "июня"},
        {7, "июля"}, {8, "августа"}, {9, "сентября"}, {10, "октября"}, {11, "ноября"}, {12, "декабря"}
    };
    return months.value(month, "");
}

QString NumericFormatWindow::dayOfWeekToWords(int dayOfWeek) const
{
    static const QMap<int, QString> days = {
        {1, "Понедельник"}, {2, "Вторник"}, {3, "Среда"}, {4, "Четверг"},
        {5, "Пятница"}, {6, "Суббота"}, {7, "Воскресенье"}
    };
    return days.value(dayOfWeek, "");
}

void NumericFormatWindow::updateTextRepresentation()
{
    QString dayOfMonthStr = numberToOrdinalWords(currentDateTime.date().day());
    QString monthName = monthToWords(currentDateTime.date().month());
    QString yearStr = yearToWords(currentDateTime.date().year());

    QString verbalDate = QString("%1 %2 %3 года, ").arg(
        dayOfMonthStr,
        monthName,
        yearStr
        );

    QString verbalTime;
    int hour = currentDateTime.time().hour();
    int minute = currentDateTime.time().minute();
    int second = currentDateTime.time().second();

    QString hourWords;
    if (hour == 0) {
        hourWords = "Двенадцать часов ночи";
    } else if (hour == 12) {
        hourWords = "Двенадцать часов дня";
    } else {
        hourWords = numberToWords(hour) + " " +
                    (hour % 10 == 1 && hour != 11 ? "час" :
                         (hour % 10 >= 2 && hour % 10 <= 4 && (hour < 10 || hour > 20) ? "часа" : "часов"));
    }

    QString minuteWords = numberToWords(minute, false, true) + " " +
                          (minute % 10 == 1 && minute != 11 ? "минута" :
                               (minute % 10 >= 2 && minute % 10 <= 4 && (minute < 10 || minute > 20) ? "минуты" : "минут"));

    QString secondWords = numberToWords(second, false, true) + " " +
                          (second % 10 == 1 && second != 11 ? "секунда" :
                               (second % 10 >= 2 && second % 10 <= 4 && (second < 10 || second > 20) ? "секунды" : "секунд"));

    verbalTime = QString("%1 %2 %3").arg(hourWords, minuteWords, secondWords);

    ui->textRepresentationEdit->setPlainText(verbalDate + verbalTime);
}

NumericFormatWindow::~NumericFormatWindow()
{
    delete timer;
    delete ui;
}
