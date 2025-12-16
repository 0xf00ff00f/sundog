#include "orbital_elements.h"

double meanAnomalyFromTrueAnomaly(const double nu, const double e)
{
    // eccentric anomaly
    const auto E = 2.0 * std::atan2(std::sqrt(1 - e) * std::sin(0.5 * nu), std::sqrt(1 + e) * std::cos(0.5 * nu));

    // mean anomaly
    return E - e * std::sin(E);
}

OrbitalElements orbitalElementsFromStateVector(const glm::dvec3 &r, const glm::dvec3 &v, JulianDate epoch, double mu)
{
    static constexpr auto kPi = glm::pi<double>();

    // magnitudes
    const auto rMag = glm::length(r);
    const auto vMag = glm::length(v);

    // specific angular momentum
    const auto h = glm::cross(r, v);
    const auto hMag = glm::length(h);

    // eccentricity vector and magnitude
    const auto eVec = (glm::cross(v, h) / mu) - (r / rMag);
    const auto e = glm::length(eVec);

    // semi-major axis
    const auto a = 1.0 / ((2.0 / rMag) - ((vMag * vMag) / mu));

    // orbit inclination
    const auto i = std::acos(h.z / hMag);

    // node vector (pointing toward ascending node)
    const auto n = glm::cross(glm::dvec3{0, 0, 1}, h);
    const auto nMag = glm::length(n);

    // right ascension of ascending node
    double Omega;
    if (nMag != 0.0)
    {
        Omega = std::acos(n.x / nMag);
        if (n.y < 0.0)
            Omega = 2.0 * kPi - Omega;
    }
    else
    {
        Omega = 0.0; // equatorial orbit
    }

    // argument of perihelion
    double omega;
    if (nMag != 0.0 && e > 1e-8)
    {
        omega = std::acos(glm::dot(n, eVec) / (nMag * e));
        if (eVec.z < 0.0)
            omega = 2.0 * kPi - omega;
    }
    else
    {
        omega = 0.0; // circular or equatorial orbit
    }

    // true anomaly
    double nu;
    if (e > 1e-8)
    {
        nu = std::acos(glm::dot(eVec, r) / (e * rMag));
        if (glm::dot(r, v) < 0.0)
            nu = 2.0 * kPi - nu;
    }
    else
    {
        nu = 0.0; // circular orbit
    }

    return OrbitalElements{.epoch = epoch,
                           .semiMajorAxis = a,
                           .eccentricity = e,
                           .inclination = i,
                           .longitudePerihelion = omega + Omega,
                           .longitudeAscendingNode = Omega,
                           .meanAnomalyAtEpoch = meanAnomalyFromTrueAnomaly(nu, e)};
}

void from_json(const nlohmann::json &json, OrbitalElements &elements)
{
    elements.epoch = JulianDate{JulianDays{json.at("epoch").get<double>()}};
    elements.semiMajorAxis = json.at("semimajor_axis").get<double>();
    elements.eccentricity = json.at("eccentricity").get<double>();
    elements.inclination = glm::radians(json.at("inclination").get<double>());
    elements.longitudePerihelion = glm::radians(json.at("longitude_perihelion").get<double>());
    elements.longitudeAscendingNode = glm::radians(json.at("longitude_ascending_node").get<double>());
    elements.meanAnomalyAtEpoch = glm::radians(json.at("mean_anomaly").get<double>());
}
