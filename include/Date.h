#ifndef DATE_H
#define DATE_H

#include "Types.h"

struct tm;

class Date 
{
public：
    struct YearMonthDay
    {
        int year; //[1900..2500]
        int month; //[1..12]
        int day; //[1..31]
    };
    static const int kDaysPerWeek = 7;
    static const int kJulianDayof1970_01_01;

    ///
    ///Construct an invalid Date.
    ///
    Date()
    : M_julianDayNumber(0)
    { }

    ///
    /// Constructs a yyyy-mm--dd Date.
    ///
    /// 1<= month <= 12

    Date(int year, int month, int day);

    ///
    /// Constructs a Date from Julian Day Number. 
    ///
    explicit Date(int julianDayNum)
        : M_julianDayNumber(julianDayNum)
    { }

    ///
    /// Construct a Date from struct tm
    ///
    explicit Date(const struct tm&);

    void swap(Date& that)
    {
        std::swap(M_julianDayNumber, that.M_julianDayNumber);
    }

    bool valid() const 
    {
        return M_julianDayNumber > 0;
    }

    string toIsoString() const;

    struct YearMonthDay yearMonthDay() const;

    int year()  const 
    {
        return yearMonthDay().year;
    }

    int month() const 
    {
        return yearMonthDay().month;
    }

    int day() const 
    {
        return yearMonthDay().day;
    }

    //[0,1,..,6] => [Sunday, Monday, ..., Saturday]
    int weekDay() const 
    {
        return (M_julianDayNumber+1) % kDaysPerWeek;
    }

    int julianDayNumber() const 
    {
        return M_julianDayNumber;
    }

private:
    int M_julianDayNumber;
};

inline bool operator<(Date x, Date y)
{
    return x.julianDayNumber() < y.julianDayNumber();
}

inline bool operator==(Date x, Date y)
{
    return x.julianDayNumber() == y.julianDayNumber();
}

#endif