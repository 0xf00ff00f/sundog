#pragma once

#include "orbital_elements.h"

#include <base/window_base.h> // FIXME for Seconds, put it somewhere else
#include <base/glhelpers.h>

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
    template<glm::length_t N>
    struct StateVector
    {
        glm::vec<N, double> position;
        glm::vec<N, double> velocity;
    };
    using StateVector2 = StateVector<2>;
    using StateVector3 = StateVector<3>;

    Orbit();
    explicit Orbit(const OrbitalElements &elems);

    void setElements(const OrbitalElements &elems);
    OrbitalElements elements() const { return m_elems; }

    glm::dmat3 orbitRotationMatrix() const { return m_orbitRotationMatrix; }
    JulianDays period() const { return m_period; }
    double meanAnomaly(JulianDate when) const;      // radians
    double eccentricAnomaly(JulianDate when) const;
    double trueAnomaly(JulianDate when) const;

    glm::dvec2 positionOnOrbitPlane(JulianDate when) const; // AU
    glm::dvec3 position(JulianDate when) const;             // AU

    StateVector2 stateVectorOnOrbitPlane(JulianDate when) const; // {AU, AU/day}
    StateVector3 stateVector(JulianDate when) const;             // {AU, AU/day}

private:
    void updatePeriod();
    void updateOrbitRotationMatrix();

    OrbitalElements m_elems;
    JulianDays m_period{0.0};
    glm::dmat3 m_orbitRotationMatrix;
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
    explicit World(const Universe *universe, const OrbitalElements &elems);

    const Universe *universe() const { return m_universe; }
    const Orbit &orbit() const { return m_orbit; }
    std::span<const MarketItemPrice> marketItemPrices() const { return m_marketItemPrices; }
    const MarketItemPrice *findMarketItemPrice(const MarketItem *item) const;

    void update();

    glm::dvec2 currentPositionOnOrbitPlane() const { return m_currentPositionOnOrbitPlane; }
    glm::dvec3 currentPosition() const { return m_currentPosition; }

    std::string name;
    double radius; // km
    JulianDays rotationPeriod;
    double axialTilt; // radians
    std::string marketName;
    std::string diffuseTexture;

private:
    const Universe *m_universe{nullptr};
    // TODO: replace this with std::unordered_map<const MarketItem *, Price>?
    // TODO: change API to something like `std::optional<Price> price(const MarketItem *item) const`
    // to make it easier to build the market snapshot table from World/Ship?
    std::vector<MarketItemPrice> m_marketItemPrices;
    Orbit m_orbit;
    glm::dvec2 m_currentPositionOnOrbitPlane;
    glm::dvec3 m_currentPosition;
};

struct MissionPlan
{
    const World *origin{nullptr};
    const World *destination{nullptr};
    JulianDate departureDate;
    JulianDate arrivalDate;
    Orbit orbit;
    double deltaVDeparture{0.0f}; // AU/day
    double deltaVArrival{0.0f};   // AU/day

    JulianDays transitTime() const { return arrivalDate - departureDate; }
};

class Ship
{
public:
    enum class State
    {
        Docked,
        InTransit
    };

    explicit Ship(const Universe *universe, const ShipClass *shipClass, const World *initialWorld);
    ~Ship();

    const Universe *universe() const { return m_universe; }
    const ShipClass *shipClass() const { return m_shipClass; }

    const World *world() const; // if State == Docked
    const Orbit *orbit() const; // if State == InTransit

    void update();

    glm::dvec3 currentPosition() const { return m_currentPosition; }
    State state() const { return m_state; }

    void setMissionPlan(std::optional<MissionPlan> missionPlan);
    const std::optional<MissionPlan> &missionPlan() const;

    int totalCargo() const;
    int cargoCapacity() const;

    struct ItemCargo
    {
        const MarketItem *item;
        int cargo;
    };

    const auto cargo() const
    {
        return m_cargo | std::views::transform([](const auto &item) -> ItemCargo {
                   return ItemCargo{item.first, item.second};
               });
    }

    int cargo(const MarketItem *item) const;
    void changeCargo(const MarketItem *item, int count);

    muslots::Signal<State> stateChangedSignal;
    muslots::Signal<const MarketItem *> cargoChangedSignal;

    std::string name;

private:
    void setState(State state);

    const Universe *m_universe{nullptr};
    const ShipClass *m_shipClass{nullptr};
    const World *m_world{nullptr};
    State m_state{State::Docked};
    std::optional<MissionPlan> m_missionPlan;
    std::unordered_map<const MarketItem *, int> m_cargo;
    glm::dvec3 m_currentPosition;
};

struct Universe
{
public:
    Universe();

    bool load(const std::string &path);

    void setDate(JulianDate date);
    JulianDate date() const { return m_date; }

    void update(Seconds elapsed);

    auto worlds() const { return m_worlds | std::views::transform(&std::unique_ptr<World>::get); }

    auto ships() const { return m_ships | std::views::transform(&std::unique_ptr<Ship>::get); }

    auto marketSectors() const
    {
        return m_marketSectors |
               std::views::transform([](const auto &sector) -> const MarketSector * { return sector.get(); });
    }

    auto shipClasses() const
    {
        return m_shipClasses |
               std::views::transform([](const auto &shipClass) -> const ShipClass * { return shipClass.get(); });
    }

    Ship *addShip(const ShipClass *shipClass, const World *world, std::string_view name);

    muslots::Signal<JulianDate> dateChangedSignal;
    muslots::Signal<Ship *> shipAddedSignal;
    muslots::Signal<Ship *> shipAboutToBeRemovedSignal;

private:
    JulianDate m_date{};
    std::vector<std::unique_ptr<MarketSector>> m_marketSectors;
    std::vector<std::unique_ptr<ShipClass>> m_shipClasses;
    std::vector<std::unique_ptr<World>> m_worlds;
    std::vector<std::unique_ptr<Ship>> m_ships;
};
