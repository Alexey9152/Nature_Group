#include "textformatwindow.h"
#include "ui_textformatwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <QRegularExpression>
#include <QStringList>
#include <QMap>

TextFormatWindow::TextFormatWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TextFormatWindow)
{
    ui->setupUi(this);
    setWindowTitle("Словесный формат");

    currentDateTime = QDateTime::currentDateTime();
    ui->textInput->setPlainText("Двадцать девятого февраля две тысячи двадцать четвого года Пятнадцать часов тридцать семь минут одна секунда");
    updateDisplay();
    connect(ui->convertBtn, &QPushButton::clicked,
            this, &TextFormatWindow::onConvertClicked);
}

void TextFormatWindow::setCustomDateTime(const QDateTime& dt)
{
    currentDateTime = dt;
    updateDisplay();
}

void TextFormatWindow::onConvertClicked()
{
    QString verbalText = ui->textInput->toPlainText().toLower();
    QDateTime parsedDateTime = parseVerbalDateTime(verbalText);

    if (parsedDateTime.isValid()) {
        currentDateTime = parsedDateTime;
        updateDisplay();
        QMessageBox::information(this, "Конвертация", "Дата и время успешно конвертированы!");
    } else {
    }
}

void TextFormatWindow::updateDisplay()
{
    ui->numericOutput->setPlainText(currentDateTime.toString("dd.MM.yyyy hh:mm:ss"));
}

int TextFormatWindow::wordToNumber(const QString& word) const
{
    static const QMap<QString, int> unitWords = {
        {"ноль", 0}, {"один", 1}, {"одна", 1}, {"два", 2}, {"две", 2}, {"три", 3}, {"четыре", 4}, {"пять", 5},
        {"шесть", 6}, {"семь", 7}, {"восемь", 8}, {"девять", 9}
    };
    static const QMap<QString, int> teenWords = {
        {"десять", 10}, {"одиннадцать", 11}, {"двенадцать", 12}, {"тринадцать", 13}, {"четырнадцать", 14},
        {"пятнадцать", 15}, {"шестнадцать", 16}, {"семнадцать", 17}, {"восемнадцать", 18}, {"девятнадцать", 19}
    };
    static const QMap<QString, int> tenWords = {
        {"двадцать", 20}, {"тридцать", 30}, {"сорок", 40}, {"пятьдесят", 50},
        {"шестьдесят", 60}, {"семьдесят", 70}, {"восемьдесят", 80}, {"девяносто", 90}
    };
    static const QMap<QString, int> hundredWords = {
        {"сто", 100}, {"двести", 200}, {"триста", 300}, {"четыреста", 400}, {"пятьсот", 500},
        {"шестьсот", 600}, {"семьсот", 700}, {"восемьсот", 800}, {"девятьсот", 900}
    };

    QString cleanedWord = word.toLower().trimmed();

    static const QMap<QString, int> ordinalNumbers = {
        {"первого", 1}, {"второго", 2}, {"третьего", 3}, {"четвертого", 4}, {"пятого", 5},
        {"шестого", 6}, {"седьмого", 7}, {"восьмого", 8}, {"девятого", 9}, {"десятого", 10},
        {"одиннадцатого", 11}, {"двенадцатого", 12}, {"тринадцатого", 13}, {"четырнадцатого", 14},
        {"пятнадцатого", 15}, {"шестнадцатого", 16}, {"семнадцатого", 17}, {"восемнадцатого", 18},
        {"девятнадцатого", 19}, {"двадцатого", 20}, {"двадцать первого", 21}, {"двадцать второго", 22},
        {"двадцать третьего", 23}, {"двадцать четвертого", 24}, {"двадцать пятого", 25},
        {"двадцать шестого", 26}, {"двадцать седьмого", 27}, {"двадцать восьмого", 28},
        {"двадцать девятого", 29}, {"тридцатого", 30}, {"тридцать первого", 31},
        {"тысячного", 1000}, {"двухтысячного", 2000}, {"двадцать четыре", 24},{"ноль", 0},
        {"две тысячи", 2000}
    };

    if (ordinalNumbers.contains(cleanedWord)) {
        return ordinalNumbers[cleanedWord];
    }

    if (unitWords.contains(cleanedWord)) return unitWords[cleanedWord];
    if (teenWords.contains(cleanedWord)) return teenWords[cleanedWord];
    if (tenWords.contains(cleanedWord)) return tenWords[cleanedWord];
    if (hundredWords.contains(cleanedWord)) return hundredWords[cleanedWord];

    bool ok;
    int num = cleanedWord.toInt(&ok);
    if (ok) return num;

    return -1;
}

int TextFormatWindow::parseNumberFromVerbal(const QStringList& words) const
{
    int result = 0;
    int currentSum = 0;

    for (const QString& word : words) {
        QString currentWord = word.toLower();
        int val = wordToNumber(currentWord);

        qDebug() << "parseNumberFromVerbal: Word '" << currentWord << "' -> Value: " << val;

        if (val != -1) {
            if (val >= 100) {
                currentSum += val;
            }
            else if (val >= 10) {
                currentSum += val;
            }
            else {
                currentSum += val;
            }
        } else {
            if (currentWord == "тысяча" || currentWord == "тысячи" || currentWord == "тысяч") {
                if (currentSum == 0) {
                    result += 1000;
                } else {
                    result += currentSum * 1000;
                }
                currentSum = 0;
            } else {
                qDebug() << "Non-numeric word encountered (ignored):" << currentWord;
            }
        }
    }
    result += currentSum;

    qDebug() << "parseNumberFromVerbal: Words:" << words.join(" ") << " -> Final Result:" << result;
    return result;
}

int TextFormatWindow::monthWordToNumber(const QString& monthWord) const
{
    static const QMap<QString, int> months = {
        {"января", 1}, {"февраля", 2}, {"марта", 3}, {"апреля", 4}, {"мая", 5}, {"июня", 6},
        {"июля", 7}, {"августа", 8}, {"сентября", 9}, {"октября", 10}, {"ноября", 11}, {"декабря", 12}
    };
    return months.value(monthWord, -1);
}

QDateTime TextFormatWindow::parseVerbalDateTime(const QString& verbalText)
{
    QDate date;
    QTime time;
    QString errorMessage;

    QString cleanedText = verbalText.toLower();
    cleanedText.replace(QRegularExpression("[.,]"), " ").replace(QRegularExpression("\\s+"), " ").trimmed();

    qDebug() << "Cleaned text for parsing:" << cleanedText;

    int day = -1, month = -1, year = -1;

    QRegularExpression dateRx("([а-яё\\s]+?)\\s+(января|февраля|марта|апреля|мая|июня|июля|августа|сентября|октября|ноября|декабря)\\s+([а-яё\\s]+?)\\s+года");
    QRegularExpressionMatch match = dateRx.match(cleanedText);

    QString remainingText = cleanedText;

    if (match.hasMatch()) {
        QString dayVerbal = match.captured(1).trimmed();
        QString monthVerbal = match.captured(2).trimmed();
        QString yearVerbal = match.captured(3).trimmed();

        day = parseNumberFromVerbal(dayVerbal.split(" ", Qt::SkipEmptyParts));
        month = monthWordToNumber(monthVerbal);
        year = parseNumberFromVerbal(yearVerbal.split(" ", Qt::SkipEmptyParts));

        qDebug() << "Parsed Date - Day:" << day << "Month:" << month << "Year:" << year;

        if (day == -1 || month == -1 || year == -1) {
            errorMessage = "Не удалось распознать день, месяц или год в дате. Пожалуйста, проверьте формат.";
        } else if (!QDate::isValid(year, month, day)) {
            if (month == 2) {
                if (day == 29 && !QDate(year, 2, 29).isValid()) {
                    errorMessage = QString("Ошибка в дате: %1 февраля %2 года. Год %2 не является високосным, в феврале всего 28 дней.").arg(day).arg(year);
                } else if (day > QDate(year, month, 1).daysInMonth()) {
                    errorMessage = QString("Ошибка в дате: %1 февраля %2 года. В феврале всего %3 дней.").arg(day).arg(year).arg(QDate(year, month, 1).daysInMonth());
                } else {
                    errorMessage = QString("Ошибка в дате: %1 февраля %2 года. Некорректный день для февраля.").arg(day).arg(year);
                }
            } else {
                if (day > QDate(year, month, 1).daysInMonth()) {
                    errorMessage = QString("Ошибка в дате: %1 %2 %3 года. В %2 всего %4 дней.").arg(day).arg(monthVerbal).arg(year).arg(QDate(year, month, 1).daysInMonth());
                } else {
                    errorMessage = QString("Ошибка в дате: %1 %2 %3 года. Некорректная дата.").arg(day).arg(monthVerbal).arg(year);
                }
            }
        }

        if (errorMessage.isEmpty()) {
            date = QDate(year, month, day);
        } else {
            QMessageBox::warning(this, "Ошибка конвертации", errorMessage);
            return QDateTime();
        }

        int yearEndIndex = match.capturedEnd(0);
        if (yearEndIndex != -1 && yearEndIndex < cleanedText.length()) {
            remainingText = cleanedText.mid(yearEndIndex).trimmed();
        } else {
            remainingText = "";
        }
        qDebug() << "Remaining text after date parsing:" << remainingText;

    } else {
        QRegularExpression numDateRx("(\\d{1,2})\\.(\\d{1,2})\\.(\\d{4})");
        QRegularExpressionMatch numDateMatch = numDateRx.match(cleanedText);
        if (numDateMatch.hasMatch()) {
            day = numDateMatch.captured(1).toInt();
            month = numDateMatch.captured(2).toInt();
            year = numDateMatch.captured(3).toInt();
            if (QDate::isValid(year, month, day)) {
                date = QDate(year, month, day);
            } else {
                errorMessage = QString("Некорректная числовая дата: %1.%2.%3. Проверьте правильность дней и месяцев.").arg(day).arg(month).arg(year);
                QMessageBox::warning(this, "Ошибка конвертации", errorMessage);
                return QDateTime();
            }
            int dateEndIndex = numDateMatch.capturedEnd(0);
            if (dateEndIndex != -1 && dateEndIndex < cleanedText.length()) {
                remainingText = cleanedText.mid(dateEndIndex).trimmed();
            } else {
                remainingText = "";
            }
            qDebug() << "Remaining text after numeric date parsing:" << remainingText;

        } else {
            date = QDate::currentDate();
            qDebug() << "Date not found in text, using current date." << date.toString("dd.MM.yyyy");
        }
    }

    int hour = -1, minute = -1, second = -1;
    bool timeParsedSuccessfully = false;

    QRegularExpression hourRx("([а-яё\\s]+?)\\s+час(ов|а|)");
    QRegularExpression minuteRx("([а-яё\\s]+?)\\s+минут(ы|а|)");
    QRegularExpression secondRx("([а-яё\\s]+?)\\s+секунд(ы|а|)");

    QString currentParseText = remainingText;

    QRegularExpressionMatch mHour = hourRx.match(currentParseText);
    if (mHour.hasMatch()) {
        hour = parseNumberFromVerbal(mHour.captured(1).trimmed().split(" ", Qt::SkipEmptyParts));
        if (hour == 24) {
            hour = 0;
        } else if (hour == 12 && verbalText.contains("ночи")) {
            hour = 0;
        }
        currentParseText = currentParseText.mid(mHour.capturedEnd(0)).trimmed();
        timeParsedSuccessfully = true;
    }

    QRegularExpressionMatch mMinute = minuteRx.match(currentParseText);
    if (mMinute.hasMatch()) {
        minute = parseNumberFromVerbal(mMinute.captured(1).trimmed().split(" ", Qt::SkipEmptyParts));
        currentParseText = currentParseText.mid(mMinute.capturedEnd(0)).trimmed();
        timeParsedSuccessfully = true;
    }

    QRegularExpressionMatch mSecond = secondRx.match(currentParseText);
    if (mSecond.hasMatch()) {
        second = parseNumberFromVerbal(mSecond.captured(1).trimmed().split(" ", Qt::SkipEmptyParts));
        timeParsedSuccessfully = true;
    }

    qDebug() << "Parsed Time - Hour:" << hour << "Minute:" << minute << "Second:" << second;

    if (!timeParsedSuccessfully) {
        QRegularExpression timeNumRx("(\\d{1,2}):(\\d{1,2}):(\\d{1,2})");
        QRegularExpressionMatch timeNumMatch = timeNumRx.match(cleanedText);
        if (timeNumMatch.hasMatch()) {
            hour = timeNumMatch.captured(1).toInt();
            minute = timeNumMatch.captured(2).toInt();
            second = timeNumMatch.captured(3).toInt();
            timeParsedSuccessfully = true;
        }
    }

    if (hour < 0 || hour > 23) {
        if (timeParsedSuccessfully && hour != -1) {
            errorMessage = QString("Ошибка во времени: Часы должны быть от 0 до 23. Вы указали %1 час(ов).").arg(hour);
            QMessageBox::warning(this, "Ошибка конвертации", errorMessage);
            return QDateTime();
        }
        hour = QTime::currentTime().hour();
    }
    if (minute < 0 || minute > 59) {
        if (timeParsedSuccessfully && minute != -1) {
            errorMessage = QString("Ошибка во времени: Минуты должны быть от 0 до 59. Вы указали %1 минут.").arg(minute);
            QMessageBox::warning(this, "Ошибка конвертации", errorMessage);
            return QDateTime();
        }
        minute = QTime::currentTime().minute();
    }
    if (second < 0 || second > 59) {
        if (timeParsedSuccessfully && second != -1) {
            errorMessage = QString("Ошибка во времени: Секунды должны быть от 0 до 59. Вы указали %1 секунд.").arg(second);
            QMessageBox::warning(this, "Ошибка конвертации", errorMessage);
            return QDateTime();
        }
        second = QTime::currentTime().second();
    }

    if (QTime::isValid(hour, minute, second)) {
        time = QTime(hour, minute, second);
    } else {
        errorMessage = "Не удалось корректно распознать время. Используется текущее время.";
        QMessageBox::warning(this, "Ошибка конвертации", errorMessage);
        time = QTime::currentTime();
        return QDateTime();
    }
    return QDateTime(date, time);
}

TextFormatWindow::~TextFormatWindow()
{
    delete ui;
}
