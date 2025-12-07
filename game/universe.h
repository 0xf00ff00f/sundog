#pragma once

#include "orbital_elements.h"

#include <base/window_base.h> // FIXME for Seconds, put it somewhere else

#include <muslots/muslots.h>

#include <nlohmann/json.hpp>

struct MarketSector;

struct MarketItem
{
    const MarketSector *sector{nullptr};
    std::string name;
    std::string description;
};

struct MarketSector
{
    std::string name;
    std::vector<std::unique_ptr<MarketItem>> items;
};

struct ShipClass
{
    std::string name;
    std::string drive;
    std::size_t cargoCapacity; // units; TODO make this tons, add weight/unit for market items
    double specificImpulse;    // seconds
    double thrust;             // N
    double power;              // kW
};

class Orbit
{
public:
    Orbit();
    explicit Orbit(const OrbitalElements &elems);

    void setElements(const OrbitalElements &elems);
    OrbitalElements elements() const { return m_elems; }

    glm::mat3 orbitRotationMatrix() const { return m_orbitRotationMatrix; }
    double period() const { return m_period; }      // Earth days
    double meanAnomaly(JulianDate when) const;      // radians
    double eccentricAnomaly(JulianDate when) const; // radians
    glm::vec2 positionOnOrbitPlane(JulianDate when) const; // AU
    glm::vec2 velocityOnOrbitPlane(JulianDate when) const; // AU/day
    glm::vec3 position(JulianDate when) const;      // AU
    glm::vec3 velocity(JulianDate when) const;      // AU/day

private:
    void updatePeriod();
    void updateOrbitRotationMatrix();

    OrbitalElements m_elems;
    double m_period{0.0};
    glm::mat3 m_orbitRotationMatrix;
};

struct MarketItemPrice
{
    const MarketItem *item{nullptr};
    uint64_t sellPrice{0}; // 0: not sold
    uint64_t buyPrice{0};  // 0: not bought
};

class Universe;

class World
{
public:
    explicit World(Universe *universe, const OrbitalElements &elems);

    const Universe *universe() const { return m_universe; }
    const Orbit &orbit() const { return m_orbit; }
    std::span<const MarketItemPrice> marketItemPrices() const { return m_marketItemPrices; }
    const MarketItemPrice *findMarketItemPrice(const MarketItem *item) const;

    glm::vec3 position() const;
    glm::vec3 position(JulianDate date) const;

    glm::vec2 positionOnOrbitPlane() const;
    glm::vec2 positionOnOrbitPlane(JulianDate date) const;

    std::string name;
    double radius; // km
    JulianClock::duration rotationPeriod;
    double axialTilt; // radians
    std::string marketName;

private:
    Universe *m_universe{nullptr};
    // TODO: replace this with std::unordered_map<const MarketItem *, Price>?
    // TODO: change API to something like `std::optional<Price> price(const MarketItem *item) const`
    // to make it easier to build the market snapshot table from World/Ship?
    std::vector<MarketItemPrice> m_marketItemPrices;
    Orbit m_orbit;
};

struct MissionPlan
{
    const World *origin{nullptr};
    const World *destination{nullptr};
    JulianDate departureTime;
    JulianDate arrivalTime;
    Orbit orbit;
    float deltaV{0.0f}; // AU/day

    JulianClock::duration transitTime() const { return arrivalTime - departureTime; }
};

class Ship
{
public:
    enum class State
    {
        Docked,
        InTransit
    };

    explicit Ship(Universe *universe, const ShipClass *shipClass, const World *initialWorld);
    ~Ship();

    const Universe *universe() const { return m_universe; }

    State state() const { return m_state; }

    glm::vec3 position() const;

    const World *world() const; // if State == Docked
    const Orbit *orbit() const; // if State == InTransit

    void setMissionPlan(std::optional<MissionPlan> missionPlan);
    const std::optional<MissionPlan> &missionPlan() const;

    int totalCargo() const;
    int cargoCapacity() const;

    int cargo(const MarketItem *item) const;
    void changeCargo(const MarketItem *item, int count);

    muslots::Signal<const MarketItem *> cargoChangedSignal;

    std::string name;

private:
    void updateState(JulianDate date);

    Universe *m_universe{nullptr};
    const ShipClass *m_shipClass{nullptr};
    const World *m_world{nullptr};
    State m_state{State::Docked};
    std::optional<MissionPlan> m_missionPlan;
    std::unordered_map<const MarketItem *, int> m_cargo;
    muslots::Connection m_dateChangedConnection;
};

struct Universe
{
public:
    Universe();

    bool load(const std::string &path);

    void setDate(JulianDate date);
    JulianDate date() const { return m_date; }

    void update(Seconds elapsed);

    auto worlds() const
    {
        return m_worlds | std::views::transform([](const auto &world) -> const World * { return world.get(); });
    }

    auto ships() const
    {
        return m_ships | std::views::transform([](const auto &ship) -> const Ship * { return ship.get(); });
    }

    auto marketSectors() const
    {
        return m_marketSectors |
               std::views::transform([](const auto &sector) -> const MarketSector * { return sector.get(); });
    }

    auto shipClasses() const
    {
        return m_shipClasses |
               std::views::transform([](const auto &shipClass) -> const ShipClass * { return shipClass.get(); });
    };

    Ship *addShip(const ShipClass *shipClass, const World *world, std::string_view name);

    muslots::Signal<JulianDate> dateChangedSignal;

private:
    JulianDate m_date{};
    std::vector<std::unique_ptr<MarketSector>> m_marketSectors;
    std::vector<std::unique_ptr<ShipClass>> m_shipClasses;
    std::vector<std::unique_ptr<World>> m_worlds;
    std::vector<std::unique_ptr<Ship>> m_ships;
};
