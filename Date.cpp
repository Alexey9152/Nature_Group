#include "Date.h"

Date::Date(int d, int m, int y, int h, int min, int s)
    : day(d), month(m), year(y), hour(h), minute(min), second(s) {}

Date::Date(const Date& other)
    : day(other.day), month(other.month), year(other.year),
    hour(other.hour), minute(other.minute), second(other.second) {}

Date& Date::operator=(const Date& other) {
    if (this != &other) {
        day = other.day;
        month = other.month;
        year = other.year;
        hour = other.hour;
        minute = other.minute;
        second = other.second;
    }
    return *this;
}

bool Date::operator>(const Date& other) const {
    if (year != other.year) return year > other.year;
    if (month != other.month) return month > other.month;
    if (day != other.day) return day > other.day;
    if (hour != other.hour) return hour > other.hour;
    if (minute != other.minute) return minute > other.minute;
    return second > other.second;
}

bool Date::operator<(const Date& other) const {
    if (year != other.year) return year < other.year;
    if (month != other.month) return month < other.month;
    if (day != other.day) return day < other.day;
    if (hour != other.hour) return hour < other.hour;
    if (minute != other.minute) return minute < other.minute;
    return second < other.second;
}

bool Date::operator==(const Date& other) const {
    return year == other.year && month == other.month && day == other.day &&
           hour == other.hour && minute == other.minute && second == other.second;
}

bool Date::operator!=(const Date& other) const {
    return !(*this == other);
}

bool Date::operator<=(const Date& other) const {
    return *this < other || *this == other;
}

bool Date::operator>=(const Date& other) const {
    return *this > other || *this == other;
}

std::ostream& operator<<(std::ostream& os, const Date& date) {
    os << date.day << "." << date.month << "." << date.year << " "
       << date.hour << ":" << date.minute << ":" << date.second;
    return os;
}
