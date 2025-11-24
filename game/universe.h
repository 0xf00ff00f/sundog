#pragma once

#include "orbital_elements.h"

#include <nlohmann/json.hpp>

struct MarketCategory;

struct MarketItemDescription
{
    const MarketCategory *category{nullptr};
    std::string name;
    std::string description;
};

struct MarketCategory
{
    std::string name;
    std::vector<std::unique_ptr<MarketItemDescription>> items;
};

struct MarketDescription
{
    std::vector<std::unique_ptr<MarketCategory>> categories;
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

struct MarketItem
{
    const MarketItemDescription *description{nullptr};
    uint64_t sellPrice{0};
    uint64_t buyPrice{0};
};

class Universe;

class World
{
public:
    explicit World(const Universe *universe, std::string name, const OrbitalElements &elems);

    const Universe *universe() const { return m_universe; }
    std::string_view name() const { return m_name; }
    const Orbit &orbit() const { return m_orbit; }
    std::span<const MarketItem> marketItems() const { return m_marketItems; }

    glm::vec3 position(JulianDate when) const;

private:
    const Universe *m_universe{nullptr};
    std::vector<MarketItem> m_marketItems;
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

private:
    std::string m_name;
    std::optional<Transit> m_transit;
};

struct Universe
{
public:
    Universe();

    bool load(const std::string &path);

    const MarketDescription &marketDescription() const { return m_marketDescription; }

    auto worlds() const
    {
        return m_worlds | std::views::transform([](const auto &world) -> const World * { return world.get(); });
    }

    auto ships() const
    {
        return m_ships | std::views::transform([](const auto &ship) -> const Ship * { return ship.get(); });
    }

    Ship *addShip(std::string_view name);

private:
    MarketDescription m_marketDescription;
    std::vector<std::unique_ptr<World>> m_worlds;
    std::vector<std::unique_ptr<Ship>> m_ships;
};
