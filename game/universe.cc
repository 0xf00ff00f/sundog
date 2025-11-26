#include "universe.h"

#include <base/file.h>

#include <glm/gtx/transform.hpp>

#include <random>

// https://farside.ph.utexas.edu/teaching/celestial/Celestial/node34.html
// http://astro.if.ufrgs.br/trigesf/position.html
// http://www.davidcolarusso.com/astro/

Orbit::Orbit() = default;

Orbit::Orbit(const OrbitalElements &elems)
    : m_elems(elems)
{
    updatePeriod();
    updateOrbitRotationMatrix();
}

void Orbit::setElements(const OrbitalElements &elems)
{
    m_elems = elems;
    updatePeriod();
    updateOrbitRotationMatrix();
}

double Orbit::meanAnomaly(JulianDate when) const
{
    const double Mepoch = m_elems.meanAnomalyAtEpoch;
    return Mepoch + 2.0 * glm::pi<double>() * (when - m_elems.epoch).count() / m_period;
}

double Orbit::eccentricAnomaly(JulianDate when) const
{
    const auto e = m_elems.eccentricity;
    const auto M = meanAnomaly(when);

    constexpr auto kTolerance = glm::radians(0.01);
    constexpr auto kMaxIterations = 200;

    double Eprev = M + e * std::sin(M) * (1.0 - e * std::cos(M));
    double E;
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

World::World(const Universe *universe, std::string name, const OrbitalElements &elems)
    : m_universe(universe)
    , m_name(std::move(name))
    , m_orbit(elems)
{
    std::random_device rnd;
    for (const auto *sector : m_universe->marketSectors())
    {
        for (const auto &item : sector->items)
        {
            const auto buyPrice = rnd() % 50000 + 5000;
            const auto sellPrice = rnd() % 50000 + 5000;
            m_marketItems.emplace_back(item.get(), buyPrice, sellPrice);
        }
    }
}

glm::vec3 World::position(JulianDate when) const
{
    return m_orbit.position(when);
}

Ship::Ship(std::string_view name)
    : m_name(name)
{
}

void Ship::setName(std::string_view name)
{
    m_name = name;
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

Ship *Universe::addShip(std::string_view name)
{
    m_ships.push_back(std::make_unique<Ship>(name));
    return m_ships.back().get();
}

bool Universe::load(const std::string &path)
{
    const auto jsonData = readFile(path);
    if (jsonData.empty())
        return false;

    const nlohmann::json json = nlohmann::json::parse(jsonData);

    // market
    for (const nlohmann::json &sectorJson : json.at("market").at("sectors"))
    {
        auto &sector = m_marketSectors.emplace_back(std::make_unique<MarketSector>());
        sector->name = sectorJson.at("name").get<std::string>();
        for (const nlohmann::json &itemJson : sectorJson.at("items"))
        {
            auto &item = sector->items.emplace_back(std::make_unique<MarketItemDescription>());
            item->sector = sector.get();
            item->name = itemJson.at("name").get<std::string>();
            item->description = itemJson.at("description").get<std::string>();
        }
    }

    // worlds
    for (const nlohmann::json &worldJson : json.at("worlds"))
    {
        m_worlds.emplace_back(std::make_unique<World>(this, worldJson.at("name").get<std::string>(),
                                                      worldJson.at("orbit").get<OrbitalElements>()));
    }

    return true;
}
