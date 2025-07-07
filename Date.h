#ifndef DATE_H
#define DATE_H

#include <iostream>
#include <string>
#include <ctime>
#include <sstream>

class Date {
public:
    Date(int d = 1, int m = 1, int y = 1970, int h = 0, int min = 0, int s = 0);
    Date(const Date& other);
    ~Date() = default;

    // Операторы сравнения
    bool operator>(const Date& other) const;
    bool operator<(const Date& other) const;
    bool operator==(const Date& other) const;
    bool operator!=(const Date& other) const;
    bool operator<=(const Date& other) const;
    bool operator>=(const Date& other) const;

    // Оператор присваивания
    Date& operator=(const Date& other);

    // Оператор вывода в стандартный поток
    friend std::ostream& operator<<(std::ostream& os, const Date& date);

    // Метод для преобразования в строку
    std::string toString() const;

    // Статический метод для получения текущей даты и времени
    static Date now();

private:
    int day, month, year;
    int hour, minute, second;
};

#endif // DATE_H
