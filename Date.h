#ifndef DATE_H
#define DATE_H

#include <iostream>
#include <ctime>

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

    // Оператор вывода
    friend std::ostream& operator<<(std::ostream& os, const Date& date);

private:
    int day, month, year;
    int hour, minute, second;
};

#endif // DATE_H
