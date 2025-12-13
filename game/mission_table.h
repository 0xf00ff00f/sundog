#pragma once

#include "universe.h"

struct MissionTable
{
public:
    struct DateState
    {
        JulianDate date;
        glm::dvec3 worldPosition;
        glm::dvec3 worldVelocity;
    };

    struct OrbitDeltaV
    {
        glm::dvec3 velDeparture;
        glm::dvec3 velArrival;
        double deltaVDeparture;
        double deltaVArrival;
    };

    const World *origin() const { return m_origin; }
    const World *destination() const { return m_destination; }
    std::vector<DateState> departures;
    std::vector<DateState> arrivals;
    std::vector<std::optional<OrbitDeltaV>> transferOrbits;

    explicit MissionTable(const World *origin, const World *destination, JulianDate start);

private:
    const World *m_origin{nullptr};
    const World *m_destination{nullptr};
};
