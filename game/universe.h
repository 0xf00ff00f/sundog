#pragma once

#include "orbital_elements.h"

#include <nlohmann/json.hpp>

struct Orbit
{
public:
    Orbit();

    void setOrbitalElements(const OrbitalElements &orbit);
    OrbitalElements orbitalElements() const { return m_orbit; }
    glm::mat3 orbitRotationMatrix() const { return m_orbitRotationMatrix; }

    float meanAnomaly(JulianDate when) const;      // radians
    float eccentricAnomaly(JulianDate when) const; // radians
    glm::vec3 position(JulianDate when) const;

private:
    void updatePeriod();
    void updateOrbitRotationMatrix();

    OrbitalElements m_orbit;
    float m_period = 0.0f;
    glm::mat3 m_orbitRotationMatrix;
};

struct World
{
public:
    World();

    void load(const nlohmann::json &json);

    std::string_view name() const { return m_name; }
    const Orbit &orbit() const { return m_orbit; }

private:
    std::string m_name;
    Orbit m_orbit;
};

struct Universe
{
public:
    Universe();

    bool load(const nlohmann::json &json);

    std::span<const World> worlds() const;

private:
    std::vector<World> m_worlds;
};
