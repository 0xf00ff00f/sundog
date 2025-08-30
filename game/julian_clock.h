#pragma once

#include <chrono>

struct JulianClock
{
    using rep = float;
    using period = std::ratio<60 * 60 * 24>;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<JulianClock>;

    static time_point now()
    {
        return time_point{duration{std::chrono::system_clock::now().time_since_epoch()} + duration{2440587.5}};
    }
};

using JulianDate = std::chrono::time_point<JulianClock>;
