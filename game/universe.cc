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
    // assuming kGMSun = (4.0 * pi^2) AU^3/years^2
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

World::World(Universe *universe, std::string name, const OrbitalElements &elems)
    : m_universe(universe)
    , m_name(std::move(name))
    , m_orbit(elems)
{
    std::random_device rnd;
    for (const auto *sector : m_universe->marketSectors())
    {
        for (const auto &item : sector->items)
        {
            const bool bought = static_cast<bool>(rnd() % 2);
            const bool sold = static_cast<bool>(rnd() % 2);
            if (!bought && !sold)
                continue;
            const auto buyPrice = bought ? rnd() % 50000 + 5000 : 0;
            const auto sellPrice = sold ? rnd() % 50000 + 5000 : 0;
            m_marketItemPrices.emplace_back(item.get(), buyPrice, sellPrice);
        }
    }
}

glm::vec3 World::position() const
{
    return position(m_universe->date());
}

glm::vec3 World::position(JulianDate date) const
{
    return m_orbit.position(date);
}

const MarketItemPrice *World::findMarketItemPrice(const MarketItem *item) const
{
    auto it = std::ranges::find_if(m_marketItemPrices, [item](const auto &price) { return price.item == item; });
    return it != m_marketItemPrices.end() ? &*it : nullptr;
}

Ship::Ship(Universe *universe, const World *world, std::string_view name)
    : m_universe(universe)
    , m_world(world)
    , m_name(name)
    , m_dateChangedConnection(m_universe->dateChangedSignal.connect([this](JulianDate date) { updateState(date); }))
{
}

Ship::~Ship()
{
    m_dateChangedConnection.disconnect();
}

void Ship::setName(std::string_view name)
{
    m_name = name;
}

void Ship::setMissionPlan(std::optional<MissionPlan> missionPlan)
{
    m_missionPlan = std::move(missionPlan);
}

const std::optional<MissionPlan> &Ship::missionPlan() const
{
    return m_missionPlan;
}

size_t Ship::totalCargo() const
{
    return std::ranges::fold_left(m_cargo, std::size_t{0},
                                  [](std::size_t count, const auto &item) { return count + item.second; });
}

size_t Ship::cargo(const MarketItem *item) const
{
    auto it = m_cargo.find(item);
    return it != m_cargo.end() ? it->second : 0;
}

void Ship::addCargo(const MarketItem *item, size_t count)
{
    auto it = m_cargo.find(item);
    if (it == m_cargo.end())
        it = m_cargo.insert(it, {item, 0});
    it->second += count;
}

void Ship::removeCargo(const MarketItem *item, size_t count)
{
    auto it = m_cargo.find(item);
    if (it == m_cargo.end())
        return;
    it->second -= count;
    if (it->second <= 0)
        m_cargo.erase(it);
}

void Ship::updateState(JulianDate date)
{
    switch (m_state)
    {
    case State::Docked: {
        assert(m_world != nullptr);
        if (m_missionPlan.has_value() && m_missionPlan->departureTime < date && date < m_missionPlan->arrivalTime)
        {
            assert(m_world == m_missionPlan->origin); // sanity check
            // started mission
            m_state = State::InTransit;
            m_world = nullptr;
        }
        break;
    }
    case State::InTransit: {
        assert(m_missionPlan.has_value());
        if (m_missionPlan->arrivalTime < date)
        {
            // arrived at destination
            m_state = State::Docked;
            m_world = m_missionPlan->destination;
            m_missionPlan.reset();
        }
        break;
    }
    }
}

glm::vec3 Ship::position() const
{
    switch (m_state)
    {
    case State::Docked: {
        assert(m_world != nullptr);
        return m_world->position();
    }
    case State::InTransit: {
        assert(m_missionPlan.has_value());
        return m_missionPlan->orbit.position(m_universe->date());
    }
    }
}

const World *Ship::world() const
{
    if (m_state != State::Docked)
        return nullptr;
    assert(m_world != nullptr);
    return m_world;
}

const Orbit *Ship::orbit() const
{
    if (m_state != State::InTransit)
        return nullptr;
    assert(m_missionPlan.has_value());
    return &m_missionPlan->orbit;
}

Universe::Universe() = default;

void Universe::setDate(JulianDate date)
{
    if (date == m_date)
        return;
    m_date = date;
    dateChangedSignal(m_date);
}

void Universe::update(Seconds elapsed)
{
    setDate(m_date + elapsed);
}

Ship *Universe::addShip(const World *world, std::string_view name)
{
    m_ships.push_back(std::make_unique<Ship>(this, world, name));
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
            auto &item = sector->items.emplace_back(std::make_unique<MarketItem>());
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
