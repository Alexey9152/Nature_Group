#include "Date.h"
#include <iomanip>

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

bool Date::operator<(const Date& other) const { return other > *this; }
bool Date::operator==(const Date& other) const {
    return year == other.year && month == other.month && day == other.day &&
           hour == other.hour && minute == other.minute && second == other.second;
}
bool Date::operator!=(const Date& other) const { return !(*this == other); }
bool Date::operator<=(const Date& other) const { return !(*this > other); }
bool Date::operator>=(const Date& other) const { return !(*this < other); }

std::ostream& operator<<(std::ostream& os, const Date& date) {
    os << std::setfill('0') << std::setw(2) << date.day << "."
       << std::setw(2) << date.month << "." << std::setw(4) << date.year << " "
       << std::setw(2) << date.hour << ":" << std::setw(2) << date.minute
       << ":" << std::setw(2) << date.second;
    return os;
}

std::string Date::toString() const {
    std::ostringstream oss;
    oss << *this;
    return oss.str();
}

Date Date::now() {
    time_t t = time(nullptr);
    tm* now = localtime(&t);
    return Date(now->tm_mday, now->tm_mon + 1, now->tm_year + 1900,
                now->tm_hour, now->tm_min, now->tm_sec);
}
