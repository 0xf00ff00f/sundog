#pragma once

#include "orbital_elements.h"

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

class Orbit
{
public:
    Orbit();
    explicit Orbit(const OrbitalElements &elems);

    void setElements(const OrbitalElements &elems);
    OrbitalElements elements() const { return m_elems; }

    glm::mat3 orbitRotationMatrix() const { return m_orbitRotationMatrix; }
    double period() const { return m_period; }      // Earth years
    double meanAnomaly(JulianDate when) const;      // radians
    double eccentricAnomaly(JulianDate when) const; // radians
    glm::vec3 position(JulianDate when) const;      // AU

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
    explicit World(const Universe *universe, std::string name, const OrbitalElements &elems);

    const Universe *universe() const { return m_universe; }
    std::string_view name() const { return m_name; }
    const Orbit &orbit() const { return m_orbit; }
    std::span<const MarketItemPrice> marketItemPrices() const { return m_marketItemPrices; }
    const MarketItemPrice *findMarketItemPrice(const MarketItem *item) const;

    glm::vec3 position(JulianDate when) const;

private:
    const Universe *m_universe{nullptr};
    std::vector<MarketItemPrice> m_marketItemPrices;
    std::string m_name;
    Orbit m_orbit;
};

struct Transit
{
    const World *origin{nullptr};
    const World *destination{nullptr};
    JulianDate departureTime;
    JulianDate arrivalTime;
    Orbit orbit;

    JulianClock::duration transitTime() const { return arrivalTime - departureTime; }
};

class Ship
{
public:
    explicit Ship(std::string_view name);

    void setName(std::string_view name);
    std::string_view name() const { return m_name; }

    void setTransit(std::optional<Transit> transit);
    const std::optional<Transit> &transit() const;

    size_t totalCargo() const;
    size_t cargo(const MarketItem *item) const;

    void addCargo(const MarketItem *item, size_t count);
    void removeCargo(const MarketItem *item, size_t count);

private:
    std::string m_name;
    std::optional<Transit> m_transit;
    std::unordered_map<const MarketItem *, size_t> m_cargo;
};

struct Universe
{
public:
    Universe();

    bool load(const std::string &path);

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

    Ship *addShip(std::string_view name);

private:
    std::vector<std::unique_ptr<MarketSector>> m_marketSectors;
    std::vector<std::unique_ptr<World>> m_worlds;
    std::vector<std::unique_ptr<Ship>> m_ships;
};
