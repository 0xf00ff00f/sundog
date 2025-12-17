#include "mission_table.h"

#include "lambert.h"

#include <mdspan>

template<typename T>
bool isNormal(const glm::vec<3, T> &v)
{
    return std::isnormal(v.x) && std::isnormal(v.y) && std::isnormal(v.z);
}

MissionTable::MissionTable(const World *origin, const World *destination, JulianDate start, double maxDeltaV)
    : m_origin(origin)
    , m_destination(destination)
{
    const auto &originOrbit = origin->orbit();
    const auto &destinationOrbit = destination->orbit();

    constexpr std::size_t kArrivalSamples = 400;
    constexpr std::size_t kDepartureSamples = 400;

    // const auto maxPeriod = JulianDays{std::max(originOrbit.period(), destinationOrbit.period())};
    const auto maxPeriod = 2.0 * std::min(originOrbit.period(), destinationOrbit.period());
    const auto departureStep = maxPeriod / kDepartureSamples;
    JulianDate departure = start;
    for (std::size_t i = 0; i < kDepartureSamples; ++i)
    {
        const auto [position, velocity] = m_origin->orbit().stateVector(departure);
        departures.emplace_back(departure, position, velocity);
        departure += departureStep;
    }

    // assuming GM = 4 * pi^2
    // Hohmann transfer: tH = pi * sqrt((r1 + r2)^3 / 8 * GM)
    // assuming kGMSun = (4.0 * pi^2) AU^3/years^2
    const JulianDays transitHohmann = JulianYears{
        0.5 *
        std::pow(0.5 * (originOrbit.elements().semiMajorAxis + destinationOrbit.elements().semiMajorAxis), 3.0 / 2.0)};
    const auto minTransitInterval = 0.5 * transitHohmann;
    const auto maxTransitInterval = 1.5 * transitHohmann;
    const auto arrivalStep = (maxPeriod + maxTransitInterval - minTransitInterval) / kArrivalSamples;

    JulianDate arrival = start + minTransitInterval;
    for (std::size_t i = 0; i < kArrivalSamples; ++i)
    {
        const auto [position, velocity] = m_destination->orbit().stateVector(arrival);
        arrivals.emplace_back(arrival, position, velocity);
        arrival += arrivalStep;
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
                        if (deltaV < maxDeltaV)
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
