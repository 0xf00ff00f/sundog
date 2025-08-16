#pragma once

#include "mesh.h"
#include "orbital_elements.h"

#include <nlohmann/json.hpp>

struct Body
{
public:
    Body();

    void setOrbitalElements(const OrbitalElements &orbit);
    OrbitalElements orbitalElements() const { return m_orbit; }

    void renderOrbit() const;
    float meanAnomaly(JulianDate when) const;      // radians
    float eccentricAnomaly(JulianDate when) const; // radians
    glm::vec3 position(JulianDate when) const;

private:
    void updatePeriod();
    void updateOrbitRotationMatrix();
    void initializeOrbitMesh();

    static constexpr auto kOrbitVertexCount = 300;

    OrbitalElements m_orbit;
    float m_period = 0.0f;
    glm::mat3 m_orbitRotationMatrix;
    Mesh m_orbitMesh;
};

struct World
{
public:
    World();

    void load(const nlohmann::json &json);

    std::string_view name() const { return m_name; }
    const Body &body() const { return m_body; }

private:
    std::string m_name;
    Body m_body;
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
