#include "mission_table.h"

#include "lambert.h"

#include <mdspan>

template<typename T>
bool isNormal(const glm::vec<3, T> &v)
{
    return std::isnormal(v.x) && std::isnormal(v.y) && std::isnormal(v.z);
}

MissionTable::MissionTable(const World *origin, const World *destination, JulianDate start)
    : m_origin(origin)
    , m_destination(destination)
{
    constexpr JulianClock::duration kMinTransitInterval{90.0};
    constexpr JulianClock::duration kMaxTransitInterval{1.0 * 365.0};
    constexpr JulianClock::duration kMaxWait{2.0 * 365.0};
    constexpr JulianClock::duration kStep{2.0};

    for (JulianDate departure = start; departure <= start + kMaxWait; departure += kStep)
    {
        const auto [position, velocity] = m_origin->orbit().stateVector(departure);
        departures.emplace_back(departure, position, velocity);
    }

    for (JulianDate arrival = start + kMinTransitInterval; arrival <= start + kMaxWait + kMaxTransitInterval;
         arrival += kStep)
    {
        const auto [position, velocity] = m_destination->orbit().stateVector(arrival);
        arrivals.emplace_back(arrival, position, velocity);
    }

    transferOrbits.resize(departures.size() * arrivals.size());
    auto orbits = std::mdspan(transferOrbits.data(), arrivals.size(), departures.size());
    for (std::size_t i = 0; i != orbits.extent(0); ++i)
    {
        assert(i < arrivals.size());
        const auto &arrival = arrivals[i];
        for (std::size_t j = 0; j < orbits.extent(1); ++j)
        {
            assert(j < departures.size());
            const auto &departure = departures[j];
            if (arrival.date > departure.date)
            {
                const auto transitInterval = arrival.date - departure.date;
                if (auto orbit =
                        lambert_battin(kGMSun, departure.worldPosition, arrival.worldPosition, transitInterval.count()))
                {
                    const auto [velDeparture, velArrival] = *orbit;
                    // FIXME sometimes lambert_battin returns vec3{inf}
                    if (isNormal(velDeparture) && isNormal(velArrival))
                    {
                        const auto deltaVDeparture = glm::length(velDeparture - departure.worldVelocity);
                        const auto deltaVArrival = glm::length(velArrival - arrival.worldVelocity);
                        const auto deltaV = deltaVDeparture + deltaVArrival;
                        if (deltaV < 0.03)
                        {
                            assert(!std::isnan(deltaV));
                            orbits[i, j] = OrbitDeltaV{velDeparture, velArrival, deltaVDeparture, deltaVArrival};
                        }
                    }
                }
            }
        }
    }
}
