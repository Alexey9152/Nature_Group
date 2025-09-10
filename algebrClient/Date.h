#ifndef DATE_H
#define DATE_H

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

class Date
{
public:
    Date(int d = 1, int m = 1, int y = 1970, int h = 0, int min = 0, int s = 0);
    Date(const Date &other);
    ~Date() = default;

    bool operator>(const Date &other) const;
    bool operator<(const Date &other) const;
    bool operator==(const Date &other) const;
    bool operator!=(const Date &other) const;
    bool operator<=(const Date &other) const;
    bool operator>=(const Date &other) const;
    Date &operator=(const Date &other);

    friend std::ostream &operator<<(std::ostream &os, const Date &date);
    std::string toString() const;
    static Date now();

    int getYear() const { return year; }
    int getMonth() const { return month; }
    int getDay() const { return day; }
    int getHour() const { return hour; }
    int getMinute() const { return minute; }
    int getSecond() const { return second; }

private:
    int day, month, year;
    int hour, minute, second;
};

#endif // DATE_H
