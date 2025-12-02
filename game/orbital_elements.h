#pragma once

#include "julian_clock.h"

#include <nlohmann/json.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

struct OrbitalElements
{
    JulianDate epoch;
    double semiMajorAxis = 1.0;
    double eccentricity = 0.0;
    double inclination = 0.0;            // radians
    double longitudePerihelion = 0.0;    // radians
    double longitudeAscendingNode = 0.0; // radians
    double meanAnomalyAtEpoch = 0.0;     // radians
};
// longitudePerihelion = longitudeAscendingNode + argumentPerihelion

double meanAnomalyFromTrueAnomaly(const double nu, const double e);

inline constexpr auto kEarthYearInDays = 365.2425;

// Sun's gravitational parameter in AU^3/days^2
inline constexpr auto kGMSun = 4.0 * glm::pi<double>() * glm::pi<double>() / (kEarthYearInDays * kEarthYearInDays);

OrbitalElements orbitalElementsFromStateVector(const glm::dvec3 &r, const glm::dvec3 &v, JulianDate epoch,
                                               double mu = kGMSun);

void from_json(const nlohmann::json &json, OrbitalElements &elements);
