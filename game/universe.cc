#include "universe.h"

#include <base/file.h>
#include <base/asset_path.h>

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

glm::vec2 Orbit::positionOnOrbitPlane(JulianDate when) const
{
    const auto e = m_elems.eccentricity;
    const auto a = m_elems.semiMajorAxis;

    const auto E = eccentricAnomaly(when);

#if 0
    const auto b = a * std::sqrt(1.0 - e * e); // semi-minor axis
    const auto x = a * (std::cos(E) - e);
    const auto y = b * std::sin(E);
#else
    // true anomaly
    const auto nu =
        2.0f * std::atan2(std::sqrt(1.0f + e) * std::sin(0.5f * E), std::sqrt(1.0f - e) * std::cos(0.5f * E));

    // distance
    const auto r = a * (1.0f - e * std::cos(E));

    const auto x = r * std::cos(nu);
    const auto y = r * std::sin(nu);
#endif

    return {x, y};
}

glm::vec3 Orbit::position(JulianDate when) const
{
    return m_orbitRotationMatrix * glm::vec3(positionOnOrbitPlane(when), 0.0);
}

Orbit::StateVector2 Orbit::stateVectorOnOrbitPlane(JulianDate when) const
{
    const auto e = m_elems.eccentricity;
    const auto a = m_elems.semiMajorAxis;

    const auto E = eccentricAnomaly(when);

    // true anomaly
    const auto nu =
        2.0f * std::atan2(std::sqrt(1.0f + e) * std::sin(0.5f * E), std::sqrt(1.0f - e) * std::cos(0.5f * E));

    // distance
    const auto r = a * (1.0f - e * std::cos(E));

    // position
    const auto p = glm::dvec2{r * std::cos(nu), r * std::sin(nu)};

    // velocity
    const auto h = std::sqrt(kGMSun * a * (1 - e * e));
    const auto v = glm::dvec2{-kGMSun / h * std::sin(nu), kGMSun / h * (e + std::cos(nu))};

    return {p, v};
}

Orbit::StateVector3 Orbit::stateVector(JulianDate when) const
{
    const auto [position, velocity] = stateVectorOnOrbitPlane(when);
    return {m_orbitRotationMatrix * glm::vec3(position, 0.0), m_orbitRotationMatrix * glm::vec3(velocity, 0.0)};
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

World::World(Universe *universe, const OrbitalElements &elems)
    : m_universe(universe)
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
    return m_orbit.position(m_universe->date());
}

glm::vec2 World::positionOnOrbitPlane() const
{
    return m_orbit.positionOnOrbitPlane(m_universe->date());
}

const MarketItemPrice *World::findMarketItemPrice(const MarketItem *item) const
{
    auto it = std::ranges::find_if(m_marketItemPrices, [item](const auto &price) { return price.item == item; });
    return it != m_marketItemPrices.end() ? &*it : nullptr;
}

Ship::Ship(Universe *universe, const ShipClass *shipClass, const World *world)
    : m_universe(universe)
    , m_shipClass(shipClass)
    , m_world(world)
    , m_dateChangedConnection(m_universe->dateChangedSignal.connect([this](JulianDate date) { updateState(date); }))
{
}

Ship::~Ship()
{
    m_dateChangedConnection.disconnect();
}

void Ship::setMissionPlan(std::optional<MissionPlan> missionPlan)
{
    m_missionPlan = std::move(missionPlan);
}

const std::optional<MissionPlan> &Ship::missionPlan() const
{
    return m_missionPlan;
}

int Ship::totalCargo() const
{
    return std::ranges::fold_left(m_cargo, 0, [](int count, const auto &item) { return count + item.second; });
}

int Ship::cargoCapacity() const
{
    return m_shipClass->cargoCapacity;
}

int Ship::cargo(const MarketItem *item) const
{
    auto it = m_cargo.find(item);
    return it != m_cargo.end() ? it->second : 0;
}

void Ship::changeCargo(const MarketItem *item, int count)
{
    const auto currentCargo = cargo(item);
    auto updatedCargo = std::clamp(currentCargo + count, 0, cargoCapacity());
    if (updatedCargo == currentCargo)
        return;
    auto it = m_cargo.find(item);
    if (updatedCargo == 0)
    {
        // remove if 0
        if (it != m_cargo.end())
            m_cargo.erase(it);
    }
    else
    {
        if (it == m_cargo.end())
            it = m_cargo.insert(it, {item, 0});
        it->second = updatedCargo;
    }
    cargoChangedSignal(item);
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
            m_world = nullptr;
            m_state = State::InTransit;
            stateChangedSignal(m_state);
        }
        break;
    }
    case State::InTransit: {
        assert(m_missionPlan.has_value());
        if (m_missionPlan->arrivalTime < date)
        {
            // arrived at destination
            m_world = m_missionPlan->destination;
            m_missionPlan.reset();
            m_state = State::Docked;
            stateChangedSignal(m_state);
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

Ship *Universe::addShip(const ShipClass *shipClass, const World *world, std::string_view name)
{
    auto &ship = m_ships.emplace_back(std::make_unique<Ship>(this, shipClass, world));
    ship->name = name;
    shipAddedSignal(ship.get());
    return ship.get();
}

bool Universe::load(const std::string &path)
{
    const auto jsonData = readFile(path);
    if (jsonData.empty())
        return false;

    const nlohmann::json json = nlohmann::json::parse(jsonData);

    // ship classes
    for (const nlohmann::json &shipClassJson : json.at("ships").at("classes"))
    {
        constexpr auto kTonsPerUnit = 10.0;
        auto &shipClass = m_shipClasses.emplace_back(std::make_unique<ShipClass>());
        shipClass->name = shipClassJson.at("name").get<std::string>();
        shipClass->drive = shipClassJson.at("drive").get<std::string>();
        shipClass->cargoCapacity = static_cast<std::size_t>(shipClassJson.at("cargo").get<double>() / kTonsPerUnit);
        shipClass->specificImpulse = shipClassJson.at("isp").get<double>();
        shipClass->thrust = shipClassJson.at("thrust").get<double>();
        shipClass->power = shipClassJson.at("power").get<double>();
    }

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
        auto name = worldJson.at("name").get<std::string>();
        const auto radius = worldJson.at("radius").get<double>();
        const auto rotationPeriod = JulianClock::duration{worldJson.at("rotation_period").get<double>()};
        const auto axialTilt = glm::radians(worldJson.at("axial_tilt").get<double>());
        auto marketName = worldJson.at("market").get<std::string>();
        auto orbit = worldJson.at("orbit").get<OrbitalElements>();
        auto texture = worldJson.at("texture").get<std::string>();
        auto &world = m_worlds.emplace_back(std::make_unique<World>(this, orbit));
        world->name = std::move(name);
        world->radius = radius;
        world->rotationPeriod = rotationPeriod;
        world->axialTilt = axialTilt;
        world->marketName = std::move(marketName);
        world->diffuseTexture = std::move(texture);
    }

    return true;
}
