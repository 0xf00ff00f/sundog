#include "universe.h"

#include <ranges>

// https://farside.ph.utexas.edu/teaching/celestial/Celestial/node34.html
// http://astro.if.ufrgs.br/trigesf/position.html
// http://www.davidcolarusso.com/astro/

Body::Body()
{
    initializeOrbitMesh();
}

void Body::setOrbitalElements(const OrbitalElements &orbit)
{
    m_orbit = orbit;
    updatePeriod();
    updateOrbitRotationMatrix();
    initializeOrbitMesh();
}

float Body::meanAnomaly(JulianDate when) const
{
    const float Mepoch = m_orbit.meanAnomalyAtEpoch;
    return Mepoch + 2.0 * glm::pi<float>() * (when - m_orbit.epoch).count() / m_period;
}

float Body::eccentricAnomaly(JulianDate when) const
{
    const float e = m_orbit.eccentricity;
    const float M = meanAnomaly(when);

    constexpr auto kTolerance = glm::radians(0.01);
    constexpr auto kMaxIterations = 200;

    float Eprev = M + e * std::sin(M) * (1.0 - e * cos(M));
    float E;
    for (std::size_t iteration = 0; iteration < kMaxIterations; ++iteration)
    {
        E = Eprev - (Eprev - e * std::sin(Eprev) - M) / (1 - e * std::cos(Eprev));
        if (std::abs(E - Eprev) < kTolerance)
            break;
        Eprev = E;
    }

    return E;
}

glm::vec3 Body::position(JulianDate when) const
{
    const auto e = m_orbit.eccentricity;
    const auto a = m_orbit.semiMajorAxis;
    const auto b = a * std::sqrt(1.0 - e * e); // semi-minor axis

    const auto E = eccentricAnomaly(when);

    // position in orbit
    const auto x = a * (std::cos(E) - e);
    const auto y = b * std::sin(E);

    return m_orbitRotationMatrix * glm::vec3(x, y, 0.0);
}

void Body::updatePeriod()
{
    constexpr auto kEarthYearInDays = 365.2425;
    m_period = std::pow(m_orbit.semiMajorAxis, 3.0 / 2.0) * kEarthYearInDays;
}

void Body::updateOrbitRotationMatrix()
{
    const float w = m_orbit.longitudePerihelion - m_orbit.longitudeAscendingNode;
    const auto rw = glm::mat3(glm::rotate(glm::mat4(1.0), w, glm::vec3(0.0, 0.0, 1.0)));

    const float i = m_orbit.inclination;
    const auto ri = glm::mat3(glm::rotate(glm::mat4(1.0), i, glm::vec3(1.0, 0.0, 0.0)));

    const float N = m_orbit.longitudeAscendingNode;
    const auto rN = glm::mat3(glm::rotate(glm::mat4(1.0), N, glm::vec3(0.0, 0.0, 1.0)));

    m_orbitRotationMatrix = rN * ri * rw;
}

void Body::initializeOrbitMesh()
{
    const auto semiMajorAxis = m_orbit.semiMajorAxis;
    const auto semiMinorAxis = semiMajorAxis * std::sqrt(1.0 - m_orbit.eccentricity * m_orbit.eccentricity);
    const auto focus = std::sqrt(semiMajorAxis * semiMajorAxis - semiMinorAxis * semiMinorAxis);

    struct Vertex
    {
        glm::vec3 position;
    };
    // clang-format off
    const std::vector<Vertex> verts =
        std::views::iota(0, kOrbitVertexCount)
        | std::views::transform([this, semiMajorAxis, semiMinorAxis, focus](const std::size_t i) -> Vertex {
              const auto t = i * 2.0f * glm::pi<float>() / kOrbitVertexCount;
              const auto x = semiMajorAxis * glm::cos(t) - focus;
              const auto y = semiMinorAxis * glm::sin(t);
              const auto position = m_orbitRotationMatrix * glm::vec3(x, y, 0.0f);
              return Vertex{.position = position};
          })
        | std::ranges::to<std::vector>();
    // clang-format on
    m_orbitMesh.setVertexData(std::as_bytes(std::span{verts}));

    const std::array<Mesh::VertexAttribute, 1> attributes = {
        Mesh::VertexAttribute{3, Mesh::Type::Float, offsetof(Vertex, position)},
    };
    m_orbitMesh.setVertexAttributes(attributes, sizeof(Vertex));
}

void Body::renderOrbit() const
{
    m_orbitMesh.draw(Mesh::Primitive::LineLoop, 0, kOrbitVertexCount);
}

World::World() = default;

void World::load(const nlohmann::json &json)
{
    const auto name = json["name"];
    assert(name.is_string());
    m_name = name.get<std::string>();

    const auto orbit = json["orbit"];
    assert(orbit.is_object());

    OrbitalElements orbitalElements;

    const auto epoch = orbit["epoch"];
    assert(epoch.is_number());
    orbitalElements.epoch = JulianDate{JulianClock::duration{epoch.get<float>()}};

    const auto semiMajorAxis = orbit["semimajor_axis"];
    assert(semiMajorAxis.is_number());
    orbitalElements.semiMajorAxis = semiMajorAxis.get<float>();

    const auto eccentricity = orbit["eccentricity"];
    assert(eccentricity.is_number());
    orbitalElements.eccentricity = eccentricity.get<float>();

    const auto inclination = orbit["inclination"];
    assert(inclination.is_number());
    orbitalElements.inclination = glm::radians(inclination.get<float>());

    const auto longitudePerihelion = orbit["longitude_perihelion"];
    assert(longitudePerihelion.is_number());
    orbitalElements.longitudePerihelion = glm::radians(longitudePerihelion.get<float>());

    const auto longitudeAscendingNode = orbit["longitude_ascending_node"];
    assert(longitudeAscendingNode.is_number());
    orbitalElements.longitudeAscendingNode = glm::radians(longitudeAscendingNode.get<float>());

    const auto meanAnomaly = orbit["mean_anomaly"];
    assert(meanAnomaly.is_number());
    orbitalElements.meanAnomalyAtEpoch = glm::radians(meanAnomaly.get<float>());

    m_body.setOrbitalElements(orbitalElements);
}

Universe::Universe() = default;

bool Universe::load(const nlohmann::json &json)
{
    const auto worldsJson = json["worlds"];
    assert(worldsJson.is_array());

    m_worlds.reserve(worldsJson.size());

    for (const auto &worldJson : worldsJson)
    {
        World world;
        world.load(worldJson);
        m_worlds.push_back(std::move(world));
    }

    return true;
}

std::span<const World> Universe::worlds() const
{
    return m_worlds;
}
