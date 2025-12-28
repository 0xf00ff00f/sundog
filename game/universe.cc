#include "universe.h"

#include <base/file.h>
#include <base/asset_path.h>

#include <glm/gtx/transform.hpp>

#include <random>

// References:
// https://stjarnhimlen.se/comp/tutorial.html
// http://www.davidcolarusso.com/astro/
// https://farside.ph.utexas.edu/teaching/celestial/Celestial/node34.html

namespace
{

constexpr auto kTolerance = 1e-10;
constexpr auto kMaxIterations = 50;

constexpr double eccentricAnomalyElliptic(double M, double e)
{
    auto E = M;
    for (std::size_t iteration = 0; iteration < kMaxIterations; ++iteration)
    {
        const auto dE = (E - e * std::sin(E) - M) / (1.0 - e * std::cos(E));
        E -= dE;
        if (std::abs(dE) < kTolerance)
            break;
    }
    return E;
}

constexpr double eccentricAnomalyHyperbolic(double M, double e)
{
    auto H = std::log(2.0 * M / e + 1.8);
    for (std::size_t iteration = 0; iteration < kMaxIterations; ++iteration)
    {
        const auto dH = (e * std::sinh(H) - H - M) / (e * std::cosh(H) - 1.0);
        H -= dH;
        if (std::abs(dH) < kTolerance)
            break;
    }
    return H;
}

constexpr double trueAnomalyElliptic(double E, double e)
{
    return 2.0 * std::atan2(std::sqrt(1.0 + e) * std::sin(0.5 * E), std::sqrt(1.0 - e) * std::cos(0.5 * E));
}

constexpr double trueAnomalyHyperbolic(double H, double e)
{
    return 2.0 * std::atan2(std::sqrt(e + 1.0) * std::sinh(0.5 * H), std::sqrt(e - 1.0) * std::cosh(0.5 * H));
}

} // namespace

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
    // assuming kGMSun = (4.0 * pi^2) AU^3/years^2
    const auto n = 2.0 * glm::pi<double>() * std::pow(std::abs(m_elems.semiMajorAxis), -3.0 / 2.0);
    return Mepoch + JulianYears{when - m_elems.epoch}.count() * n;
}

double Orbit::eccentricAnomaly(JulianDate when) const
{
    const auto e = m_elems.eccentricity;
    const auto M = meanAnomaly(when);
    if (e < 1.0)
    {
        return eccentricAnomalyElliptic(M, e);
    }
    else
    {
        return eccentricAnomalyHyperbolic(M, e);
    }
}

double Orbit::trueAnomaly(JulianDate when) const
{
    const auto e = m_elems.eccentricity;
    const auto M = meanAnomaly(when);
    if (e < 1.0)
    {
        const auto E = eccentricAnomalyElliptic(M, e);
        return trueAnomalyElliptic(E, e);
    }
    else
    {
        const auto H = eccentricAnomalyHyperbolic(M, e);
        return trueAnomalyHyperbolic(H, e);
    }
}

glm::dvec2 Orbit::positionOnOrbitPlane(JulianDate when) const
{
    const auto e = m_elems.eccentricity;
    const auto a = m_elems.semiMajorAxis;
    const auto M = meanAnomaly(when);

    const auto [nu, r] = [this, e, a, M]() -> std::tuple<double, double> {
        if (e < 1.0)
        {
            // elliptical orbit

            // eccentric anomaly
            const auto E = eccentricAnomalyElliptic(M, e);

            // true anomaly
            const auto nu = trueAnomalyElliptic(E, e);

            // radius
            const auto r = a * (1.0 - e * std::cos(E));

            return std::make_tuple(nu, r);
        }
        else
        {
            // hyperbolic orbit

            // eccentric anomaly
            const auto H = eccentricAnomalyHyperbolic(M, e);

            // true anomaly
            const auto nu = trueAnomalyHyperbolic(H, e);

            // radius
            const auto r = a * (1.0 - e * std::cosh(H));

            return std::make_tuple(nu, r);
        }
    }();

    const auto x = r * std::cos(nu);
    const auto y = r * std::sin(nu);
    return glm::dvec2{x, y};
}

glm::dvec3 Orbit::position(JulianDate when) const
{
    return m_orbitRotationMatrix * glm::dvec3{positionOnOrbitPlane(when), 0.0};
}

Orbit::StateVector2 Orbit::stateVectorOnOrbitPlane(JulianDate when) const
{
    const auto e = m_elems.eccentricity;
    const auto a = m_elems.semiMajorAxis;
    const auto M = meanAnomaly(when);

    const auto [nu, r, p] = [this, e, a, M]() -> std::tuple<double, double, double> {
        if (e < 1.0)
        {
            // elliptical orbit

            // eccentric anomaly
            const auto E = eccentricAnomalyElliptic(M, e);

            // true anomaly
            const auto nu = trueAnomalyElliptic(E, e);

            // radius
            const auto r = a * (1.0 - e * std::cos(E));

            const auto p = a * (1.0 - e * e);

            return std::make_tuple(nu, r, p);
        }
        else
        {
            // hyperbolic orbit

            // eccentric anomaly
            const auto H = eccentricAnomalyHyperbolic(M, e);

            // true anomaly
            const auto nu = trueAnomalyHyperbolic(H, e);

            // radius
            const auto r = a * (1.0 - e * std::cosh(H));

            const auto p = std::abs(a) * (e * e - 1.0);

            return std::make_tuple(nu, r, p);
        }
    }();

    // position
    const auto x = r * std::cos(nu);
    const auto y = r * std::sin(nu);

    // velocity
    const auto h = std::sqrt(kGMSun * p);

    const auto vr = (kGMSun / h) * e * std::sin(nu);
    const auto vTheta = (kGMSun / h) * (1.0 + e * std::cos(nu));

    const auto vx = vr * std::cos(nu) - vTheta * std::sin(nu);
    const auto vy = vr * std::sin(nu) + vTheta * std::cos(nu);

    return {glm::dvec2{x, y}, glm::dvec2{vx, vy}};
}

Orbit::StateVector3 Orbit::stateVector(JulianDate when) const
{
    const auto [position, velocity] = stateVectorOnOrbitPlane(when);
    return {m_orbitRotationMatrix * glm::dvec3(position, 0.0), m_orbitRotationMatrix * glm::dvec3(velocity, 0.0)};
}

void Orbit::updatePeriod()
{
    // assuming kGMSun = (4.0 * pi^2) AU^3/years^2
    m_period = JulianYears{std::pow(m_elems.semiMajorAxis, 3.0 / 2.0)};
}

void Orbit::updateOrbitRotationMatrix()
{
    const auto w = m_elems.longitudePerihelion - m_elems.longitudeAscendingNode;
    const auto rw = glm::dmat3{glm::rotate(glm::dmat4(1.0), w, glm::dvec3(0.0, 0.0, 1.0))};

    const auto i = m_elems.inclination;
    const auto ri = glm::dmat3{glm::rotate(glm::dmat4(1.0), i, glm::dvec3(1.0, 0.0, 0.0))};

    const auto N = m_elems.longitudeAscendingNode;
    const auto rN = glm::dmat3{glm::rotate(glm::dmat4(1.0), N, glm::dvec3(0.0, 0.0, 1.0))};

    m_orbitRotationMatrix = rN * ri * rw;
}

World::World(const Universe *universe, const OrbitalElements &elems)
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

void World::update()
{
    m_currentPositionOnOrbitPlane = m_orbit.positionOnOrbitPlane(m_universe->date());
    m_currentPosition = m_orbit.orbitRotationMatrix() * glm::dvec3(m_currentPositionOnOrbitPlane, 0.0);
}

const MarketItemPrice *World::findMarketItemPrice(const MarketItem *item) const
{
    auto it = std::ranges::find_if(m_marketItemPrices, [item](const auto &price) { return price.item == item; });
    return it != m_marketItemPrices.end() ? &*it : nullptr;
}

Ship::Ship(const Universe *universe, const ShipClass *shipClass, const World *world)
    : m_universe(universe)
    , m_shipClass(shipClass)
    , m_world(world)
{
    std::random_device rnd;
    for (const auto *sector : m_universe->marketSectors())
    {
        for (const auto &item : sector->items)
        {
            if ((rnd() % 2) == 0)
                changeCargo(item.get(), rnd() % 10);
        }
    }
}

Ship::~Ship() = default;

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

void Ship::update()
{
    const auto date = m_universe->date();

    // update state

    switch (m_state)
    {
    case State::Docked: {
        assert(m_world != nullptr);
        if (m_missionPlan.has_value() && m_missionPlan->departureDate < date && date < m_missionPlan->arrivalDate)
        {
            assert(m_world == m_missionPlan->origin); // sanity check
            // started mission
            m_world = nullptr;
            setState(State::InTransit);
        }
        break;
    }
    case State::InTransit: {
        assert(m_missionPlan.has_value());
        if (m_missionPlan->arrivalDate < date)
        {
            // arrived at destination
            m_world = m_missionPlan->destination;
            m_missionPlan.reset();
            setState(State::Docked);
        }
        break;
    }
    }

    // update position

    switch (m_state)
    {
    case State::Docked: {
        assert(m_world != nullptr);
        m_currentPosition = m_world->currentPosition();
        break;
    }
    case State::InTransit: {
        assert(m_missionPlan.has_value());
        m_currentPosition = m_missionPlan->orbit.position(date);
        break;
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

void Ship::setState(State state)
{
    if (state == m_state)
        return;
    m_state = state;
    stateChangedSignal(m_state);
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

    for (auto &world : m_worlds)
        world->update();

    for (auto &ship : m_ships)
        ship->update();
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
        const auto rotationPeriod = JulianDays{worldJson.at("rotation_period").get<double>()};
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
