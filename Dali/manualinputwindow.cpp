#include "manualinputwindow.h"
#include "ui_manualinputwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QRegularExpression>

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

static const QMap<int, int> daysInMonthMap = {
    {1, 31}, {2, 28}, {3, 31}, {4, 30}, {5, 31}, {6, 30},
    {7, 31}, {8, 31}, {9, 30}, {10, 31}, {11, 30}, {12, 31}
};

static const QMap<int, QString> prepositionalMonths = {
    {1, "январе"}, {2, "феврале"}, {3, "марте"}, {4, "апреле"}, {5, "мае"}, {6, "июне"},
    {7, "июле"}, {8, "августе"}, {9, "сентябре"}, {10, "октябре"}, {11, "ноябре"}, {12, "декабре"}
};

ManualInputWindow::ManualInputWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ManualInputWindow)
{
    ui->setupUi(this);
    setWindowTitle("Ручной ввод даты и времени");

    connect(ui->inputDateTimeLineEdit, &QLineEdit::textChanged,
            this, &ManualInputWindow::on_inputDateTimeLineEdit_textChanged);
}

ManualInputWindow::~ManualInputWindow()
{
    delete ui;
}

void ManualInputWindow::on_inputDateTimeLineEdit_textChanged(const QString &text)
{
    if (text.isEmpty()) {
        ui->verbalOutputPlainTextEdit->setPlainText("Пожалуйста, введите дату и время.");
        return;
    }

    QRegularExpression regex("^(\\d{2})\\.(\\d{2})\\.(\\d{4})\\s(\\d{2}):(\\d{2}):(\\d{2})$");
    QRegularExpressionMatch match = regex.match(text);

    if (!match.hasMatch()) {
        ui->verbalOutputPlainTextEdit->setPlainText("Ошибка: Некорректный формат ввода. Ожидается: ДД.ММ.ГГГГ ЧЧ:ММ:СС");
        return;
    }

    bool okDay, okMonth, okYear, okHour, okMinute, okSecond;
    int day = match.captured(1).toInt(&okDay);
    int month = match.captured(2).toInt(&okMonth);
    int year = match.captured(3).toInt(&okYear);
    int hour = match.captured(4).toInt(&okHour);
    int minute = match.captured(5).toInt(&okMinute);
    int second = match.captured(6).toInt(&okSecond);

    if (!okDay || !okMonth || !okYear || !okHour || !okMinute || !okSecond) {
        ui->verbalOutputPlainTextEdit->setPlainText("Ошибка: Невозможно распознать числа в дате/времени.");
        return;
    }

    // --- Валидация даты ---
    if (month < 1 || month > 12) {
        ui->verbalOutputPlainTextEdit->setPlainText("Ошибка: Некорректный номер месяца (должен быть от 01 до 12).");
        return;
    }

    if (year < 1 || year > 9999) {
        ui->verbalOutputPlainTextEdit->setPlainText("Ошибка: Некорректное значение года.");
        return;
    }

    int expectedDaysInMonth = daysInMonthMap.value(month, 0);
    if (month == 2 && QDate::isLeapYear(year)) {
        expectedDaysInMonth = 29;
    }

    if (day < 1 || day > expectedDaysInMonth) {
        QString errorMsg = "Ошибка: Некорректная дата. ";

        if (month == 2) {
            if (QDate::isLeapYear(year)) {
                errorMsg += QString("В феврале %1 года (високосный) - 29 дней.").arg(year);
            } else {
                errorMsg += QString("%1 год не является високосным, в феврале 28 дней.").arg(year);
            }
        } else {
            errorMsg += QString("В %1 %2 дней.").arg(monthToPrepositionalWords(month)).arg(expectedDaysInMonth);
        }
        ui->verbalOutputPlainTextEdit->setPlainText(errorMsg);
        return;
    }
    if (hour < 0 || hour > 23) {
        ui->verbalOutputPlainTextEdit->setPlainText("Ошибка: Некорректное значение часов (должно быть от 00 до 23).");
        return;
    }

    if (minute < 0 || minute > 59) {
        ui->verbalOutputPlainTextEdit->setPlainText("Ошибка: Некорректное значение минут (должно быть от 00 до 59).");
        return;
    }

    if (second < 0 || second > 59) {
        ui->verbalOutputPlainTextEdit->setPlainText("Ошибка: Некорректное значение секунд (должно быть от 00 до 59).");
        return;
    }

    QDate tempDate(year, month, day);
    QTime tempTime(hour, minute, second);
    currentParsedDateTime = QDateTime(tempDate, tempTime);

    if (!currentParsedDateTime.isValid()) {
        ui->verbalOutputPlainTextEdit->setPlainText("Неизвестная ошибка: Не удалось создать валидный объект даты/времени.");
        return;
    }

    updateVerbalRepresentation();
}


void ManualInputWindow::updateVerbalRepresentation()
{
    QString dayOfMonthStr = numberToOrdinalWords(currentParsedDateTime.date().day());
    QString monthName = monthToWords(currentParsedDateTime.date().month());
    QString yearStr = yearToWords(currentParsedDateTime.date().year());

    QString verbalDate = QString("%1 %2 %3 года, ").arg(
        dayOfMonthStr,
        monthName,
        yearStr
        );

    QString verbalTime;
    int hour = currentParsedDateTime.time().hour();
    int minute = currentParsedDateTime.time().minute();
    int second = currentParsedDateTime.time().second();

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

    ui->verbalOutputPlainTextEdit->setPlainText(verbalDate + verbalTime);
}

QString ManualInputWindow::numberToWords(int number, bool ordinal, bool isFeminine) const
{
    if (number == 0) {
        if (ordinal) return "нулевого";
        return "ноль";
    }
    if (number < 0 || number > 9999) return QString::number(number);

    QString result;

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
    } else { // 1-9
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

QString ManualInputWindow::numberToOrdinalWords(int number) const {
    return numberToWords(number, true);
}

QString ManualInputWindow::yearToWords(int year) const {
    QString result;
    if (year < 1000) {
        return numberToOrdinalWords(year);
    } else {
        int thousands = year / 1000;
        int remainder = year % 1000;


        if (thousands == 1) {
            result += "тысяча";
        } else if (thousands == 2) {
            result += "две тысячи";
        } else if (thousands >= 5 && thousands <= 20) {
            result += numberToWords(thousands) + " тысяч";
        } else if (thousands % 10 == 1 && thousands != 11) {
            result += numberToWords(thousands) + " тысяча";
        } else if (thousands % 10 >= 2 && thousands % 10 <= 4 && (thousands < 10 || thousands > 20)) {
            result += numberToWords(thousands) + " тысячи";
        } else {
            result += numberToWords(thousands) + " тысяч";
        }
        if (remainder > 0) {
            result += " " + numberToOrdinalWords(remainder);
        }
    }
    return result.trimmed();
}

QString ManualInputWindow::monthToWords(int month) const
{
    static const QMap<int, QString> months = {
        {1, "января"}, {2, "февраля"}, {3, "марта"}, {4, "апреля"}, {5, "мая"}, {6, "июня"},
        {7, "июля"}, {8, "августа"}, {9, "сентября"}, {10, "октября"}, {11, "ноября"}, {12, "декабря"}
    };
    return months.value(month, "");
}

QString ManualInputWindow::dayOfWeekToWords(int dayOfWeek) const
{
    static const QMap<int, QString> days = {
        {1, "Понедельник"}, {2, "Вторник"}, {3, "Среда"}, {4, "Четверг"},
        {5, "Пятница"}, {6, "Суббота"}, {7, "Воскресенье"}
    };
    return days.value(dayOfWeek, "");
}

QString ManualInputWindow::monthToPrepositionalWords(int month) const
{
    return prepositionalMonths.value(month, "");
}
