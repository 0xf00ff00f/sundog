#pragma once

#include <chrono>
#include <format>

struct JulianClock
{
    using rep = double;
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

template<>
struct std::formatter<JulianDate>
{
    enum class FormatType
    {
        Raw,
        Date,
        Time
    };
    FormatType m_formatType{FormatType::Raw};

    template<typename ParseContext>
    constexpr auto parse(ParseContext &ctx)
    {
        auto it = ctx.begin();
        if (it == ctx.end())
            return it;
        switch (*it)
        {
        case 'D':
            m_formatType = FormatType::Date;
            ++it;
            break;
        case 'T':
            m_formatType = FormatType::Time;
            ++it;
            break;
        default:
            break;
        }
        if (it != ctx.end() && *it != '}')
            throw std::format_error("Invalid format for JulianDate");
        return it;
    }

    auto format(JulianDate date, std::format_context &ctx) const
    {
        switch (m_formatType)
        {
        case FormatType::Date: {
            const auto kMonths =
                std::array{"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
            const auto yearMonthDay = toYearMonthDay(date);
            return std::format_to(ctx.out(), "{:02d} {} {}", static_cast<unsigned>(yearMonthDay.day()),
                                  kMonths[static_cast<unsigned>(yearMonthDay.month()) - 1],
                                  static_cast<int>(yearMonthDay.year()));
        }
        case FormatType::Time: {
            double dummy{};
            const auto dayFraction = std::modf(date.time_since_epoch().count() - 0.5, &dummy);
            const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(JulianDate::duration{dayFraction});
            const auto hour = seconds.count() / (60 * 60);
            const auto minute = (seconds.count() / 60) % 60;
            const auto second = seconds.count() % 60;
            return std::format_to(ctx.out(), "{:02}:{:02}:{:02d}", hour, minute, second);
        }
        default:
            return std::format_to(ctx.out(), "{}", date.time_since_epoch().count());
        }
    }
};

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
