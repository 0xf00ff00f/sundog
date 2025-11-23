#pragma once

#include <chrono>

struct JulianClock
{
    using rep = float;
    using period = std::ratio<60 * 60 * 24>;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<JulianClock>;

    static constexpr auto kUnixEpoch = time_point{duration{2440587.5}};

    static time_point now() { return kUnixEpoch + duration{std::chrono::system_clock::now().time_since_epoch()}; }
};

using JulianDate = std::chrono::time_point<JulianClock>;

template<typename Duration>
constexpr JulianDate toJulianDate(std::chrono::sys_time<Duration> time)
{
    return JulianClock::kUnixEpoch + JulianClock::duration{time.time_since_epoch()};
}

template<typename Duration>
constexpr std::chrono::sys_time<Duration> toSystemTime(JulianDate date)
{
    return std::chrono::sys_time<Duration>{std::chrono::duration_cast<Duration>(date - JulianClock::kUnixEpoch)};
}

constexpr std::chrono::year_month_day toYearMonthDay(JulianDate date)
{
    return std::chrono::year_month_day{toSystemTime<std::chrono::days>(date)};
}

constexpr JulianDate toJulianDate(std::chrono::year_month_day yearMonthDay)
{
    return toJulianDate(std::chrono::sys_days{yearMonthDay});
}

// tests
static_assert([] {
    using namespace std::chrono;
    static_assert(toSystemTime<days>(toJulianDate(sys_days{})) == sys_days{});
    static_assert(toYearMonthDay(toJulianDate(sys_days{})) == year_month_day(1970y, January, 1d));
    static_assert(toYearMonthDay(JulianDate{JulianClock::duration{2451544.5}}) == year_month_day(2000y, January, 1d));
    static_assert(toJulianDate(year_month_day(2000y, January, 1d)) == JulianDate{JulianClock::duration{2451544.5}});
    static_assert(toYearMonthDay(JulianDate{JulianClock::duration{2461000.5}}) == year_month_day(2025y, November, 21d));
    static_assert(toJulianDate(year_month_day(2025y, November, 21d)) == JulianDate{JulianClock::duration{2461000.5}});
    return true;
}());
