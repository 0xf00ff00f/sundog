#include "universe.h"

#include <glm/gtx/transform.hpp>

// https://farside.ph.utexas.edu/teaching/celestial/Celestial/node34.html
// http://astro.if.ufrgs.br/trigesf/position.html
// http://www.davidcolarusso.com/astro/

Orbit::Orbit() = default;

void Orbit::setElements(const OrbitalElements &elems)
{
    m_elems = elems;
    updatePeriod();
    updateOrbitRotationMatrix();
}

float Orbit::meanAnomaly(JulianDate when) const
{
    const float Mepoch = m_elems.meanAnomalyAtEpoch;
    return Mepoch + 2.0 * glm::pi<float>() * (when - m_elems.epoch).count() / m_period;
}

float Orbit::eccentricAnomaly(JulianDate when) const
{
    const float e = m_elems.eccentricity;
    const float M = meanAnomaly(when);

    constexpr auto kTolerance = glm::radians(0.01);
    constexpr auto kMaxIterations = 200;

    float Eprev = M + e * std::sin(M) * (1.0 - e * cos(M));
    float E;
    for (std::size_t iteration = 0; iteration < kMaxIterations; ++iteration)
    {
        E = Eprev - (Eprev - e * std::sin(Eprev) - M) / (1 - e * std::cos(Eprev));
        if (std::abs(E - Eprev) < kTolerance)
            break;
        Eprev = E;
    }

    return E;
}

glm::vec3 Orbit::position(JulianDate when) const
{
    const auto e = m_elems.eccentricity;
    const auto a = m_elems.semiMajorAxis;
    const auto b = a * std::sqrt(1.0 - e * e); // semi-minor axis

    const auto E = eccentricAnomaly(when);

    // position in orbit
    const auto x = a * (std::cos(E) - e);
    const auto y = b * std::sin(E);

    return m_orbitRotationMatrix * glm::vec3(x, y, 0.0);
}

void Orbit::updatePeriod()
{
    constexpr auto kEarthYearInDays = 365.2425;
    m_period = std::pow(m_elems.semiMajorAxis, 3.0 / 2.0) * kEarthYearInDays;
}

void Orbit::updateOrbitRotationMatrix()
{
    const float w = m_elems.longitudePerihelion - m_elems.longitudeAscendingNode;
    const auto rw = glm::mat3(glm::rotate(glm::mat4(1.0), w, glm::vec3(0.0, 0.0, 1.0)));

    const float i = m_elems.inclination;
    const auto ri = glm::mat3(glm::rotate(glm::mat4(1.0), i, glm::vec3(1.0, 0.0, 0.0)));

    const float N = m_elems.longitudeAscendingNode;
    const auto rN = glm::mat3(glm::rotate(glm::mat4(1.0), N, glm::vec3(0.0, 0.0, 1.0)));

    m_orbitRotationMatrix = rN * ri * rw;
}

World::World(std::string name, const OrbitalElements &elems)
    : m_name(std::move(name))
{
    m_orbit.setElements(elems);
}

glm::vec3 World::position(JulianDate when) const
{
    return m_orbit.position(when);
}

Ship::Ship(std::string_view name)
    : m_name(name)
{
}

void Ship::setTransit(std::optional<Transit> transit)
{
    m_transit = std::move(transit);
}

const std::optional<Transit> &Ship::transit() const
{
    return m_transit;
}

Universe::Universe() = default;

void Universe::setWorlds(std::vector<std::unique_ptr<World>> worlds)
{
    m_worlds = std::move(worlds);
}

Ship *Universe::addShip(std::string_view name)
{
    m_ships.push_back(std::make_unique<Ship>(name));
    return m_ships.back().get();
}

namespace nlohmann
{
template<>
struct adl_serializer<std::unique_ptr<World>>
{
    static std::unique_ptr<World> from_json(const json &j)
    {
        return std::make_unique<World>(j.at("name").get<std::string>(), j.at("orbit").get<OrbitalElements>());
    }
};
} // namespace nlohmann

void from_json(const nlohmann::json &json, Universe &universe)
{
    universe.setWorlds(json.at("worlds").get<std::vector<std::unique_ptr<World>>>());
}
