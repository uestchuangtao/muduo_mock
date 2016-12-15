#include "Types.h"

#include <boost/operators.hpp>

class Timestamp : public boost::less_than_comparable<Timestamp>
{
public:
    Timestamp()
        :M_microSecondsSinceEpoch(0)
    {

    }

    explicit Timestamp(int64_t microSecondsSinceEpochArg)
        :M_microSecondsSinceEpoch(microSecondsSinceEpochArg)
    {

    }

    void swap(Timestamp& that)
    {
        std::swap(M_microSecondsSinceEpoch, that.M_microSecondsSinceEpoch);
    }


    //default copy/assginment/dtor are okay

    string toString() const;
    string toFormattedString(bool showMicroseconds = true) const;  

    bool valid() const 
    {
        return M_microSecondsSinceEpoch > 0;
    } 

    //for internal usage
    int64_t microSecondsSinceEpoch() const 
    {
        return M_microSecondsSinceEpoch;
    }

    time_t secondsSinceEpoch() const 
    {
        return static_cast<time_t>(M_microSecondsSinceEpoch / kMicroSecondsPerSecond);
    }

    ///
    ///Get time of now
    ///
    static Timestamp now();
    static Timestamp invalid()
    {
        return Timestamp();
    }

    static Timestamp fromUnixTime(time_t t)
    {
        return fromUnixTime(t, 0)；
    }

    static Timestamp fromUnixTime(time_t t, int microseconds)
    {
        return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
    }

    static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
    int64_t M_microSecondsSinceEpoch;
};

inline bool operator<(Timestamp lhs,Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs,Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

///
/// Gets time difference of two timestamps, result in seconds.
///
/// @param high, low
/// @return (high-low) in seconds
/// @c double has 52-bit precision, enough for one-microsecond
/// resolution for next 100 years.
inline double timeDifference(Timestamp high, Timestamp low)
{
    int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

///
/// Add @c seconds to given timestamp.
///
/// @return timestamp+seconds as Timestamp
///
inline Timestamp addTime(Timestamp timestamp, double seconds)
{
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}