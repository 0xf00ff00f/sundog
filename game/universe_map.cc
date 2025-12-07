#include "universe_map.h"

#include "style_settings.h"

#include <base/system.h>
#include <base/shader_manager.h>
#include <base/painter.h>
#include <base/mesh.h>

#include <glm/gtx/string_cast.hpp>

// #define ORBIT_WIREFRAME
// #define SPHERE_WIREFRAME

namespace
{

float raySphereIntersect(const glm::vec3 &rayFrom, const glm::vec3 &rayDir, const glm::vec3 &sphereCenter,
                         float sphereRadius)
{
    const glm::vec3 delta = rayFrom - sphereCenter;
    const float a = glm::dot(rayDir, rayDir);
    const float b = 2.0 * dot(rayDir, delta);
    const float c = glm::dot(delta, delta) - sphereRadius * sphereRadius;
    const float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f)
        return -1.0;
    return (-b - sqrt(discriminant)) / (2.0f * a);
}

// latitude: n/s, -pi/2 to pi/2
// longitude: e/w, -pi to pi
glm::vec3 latLonToCartesian(float lat, float lon)
{
    const auto r = std::cos(lat);
    const auto x = r * std::cos(lon);
    const auto y = r * std::sin(lon);
    const auto z = std::sin(lat);
    return {x, y, z};
}

std::unique_ptr<Mesh> createOrbitMesh()
{
    static constexpr auto kVertexCount = 120;

    struct Vertex
    {
        float meanAnomaly;
        float normalDirection;
    };

    auto mesh = std::make_unique<Mesh>();

    std::vector<Vertex> verts;
    verts.reserve(kVertexCount * 2);
    for (std::size_t i = 0; i < kVertexCount; ++i)
    {
        const auto meanAnomaly = i * 2.0f * glm::pi<float>() / (kVertexCount - 1);
        verts.emplace_back(meanAnomaly, -1.0f);
        verts.emplace_back(meanAnomaly, 1.0f);
    }
    mesh->setVertexData(std::as_bytes(std::span{verts}), verts.size());

#if defined(ORBIT_WIREFRAME)
    std::vector<uint32_t> indices;
    indices.reserve(kVertexCount * 8);
    for (std::size_t i = 0; i < kVertexCount - 1; ++i)
    {
        const auto baseIndex = i * 2;

        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 1);

        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 2);

        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);

        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 3);
    }
    mesh->setIndexData(std::span{indices});
#endif

    const std::array<Mesh::VertexAttribute, 2> attributes = {
        Mesh::VertexAttribute{1, Mesh::Type::Float, offsetof(Vertex, meanAnomaly)},
        Mesh::VertexAttribute{1, Mesh::Type::Float, offsetof(Vertex, normalDirection)}};
    mesh->setVertexAttributes(attributes, sizeof(Vertex));

    return mesh;
}

std::unique_ptr<Mesh> createSphereMesh()
{
    constexpr auto kRings = 20;
    constexpr auto kSlices = 20;

    struct Vertex
    {
        glm::vec3 position;
        glm::vec2 texCoord;
    };

    auto mesh = std::make_unique<Mesh>();

    std::vector<Vertex> verts;
    for (std::size_t i = 0; i < kRings; ++i)
    {
        const auto lat = i * glm::pi<float>() / (kRings - 1) - 0.5f * glm::pi<float>();
        const auto u = static_cast<float>(i) / (kRings - 1);
        for (std::size_t j = 0; j < kSlices; ++j)
        {
            const auto lon = j * 2.0f * glm::pi<float>() / kSlices - glm::pi<float>();
            const auto v = static_cast<float>(j) / kSlices;

            const auto position = latLonToCartesian(lat, lon);
            const auto texCoord = glm::vec2{v, u};

            verts.emplace_back(position, texCoord);
        }
    }
    mesh->setVertexData(std::as_bytes(std::span{verts}), verts.size());

    std::vector<uint32_t> indices;
    for (std::size_t i = 0; i < kRings - 1; ++i)
    {
        for (std::size_t j = 0; j < kSlices; ++j)
        {
            const auto v0 = i * kSlices + j;
            const auto v1 = (i + 1) * kSlices + j;
            const auto v2 = (i + 1) * kSlices + (j + 1) % kSlices;
            const auto v3 = i * kSlices + (j + 1) % kSlices;

#if !defined(SPHERE_WIREFRAME)
            indices.push_back(v0);
            indices.push_back(v1);
            indices.push_back(v2);

            indices.push_back(v2);
            indices.push_back(v3);
            indices.push_back(v0);
#else
            indices.push_back(v0);
            indices.push_back(v1);

            indices.push_back(v1);
            indices.push_back(v2);

            indices.push_back(v2);
            indices.push_back(v3);

            indices.push_back(v3);
            indices.push_back(v0);
#endif
        }
    }
    mesh->setIndexData(indices);

    const std::array<Mesh::VertexAttribute, 2> attributes = {
        Mesh::VertexAttribute{3, Mesh::Type::Float, offsetof(Vertex, position)},
        Mesh::VertexAttribute{2, Mesh::Type::Float, offsetof(Vertex, texCoord)}};
    mesh->setVertexAttributes(attributes, sizeof(Vertex));

    return mesh;
}

std::unique_ptr<Mesh> createBodyBillboardMesh()
{
    static constexpr auto kCircleMeshVertexCount = 20;
    constexpr auto kRadius = 0.05;

    // clang-format off
    const std::vector<glm::vec2> verts = std::views::iota(0, kCircleMeshVertexCount)
                                         | std::views::transform([](const std::size_t i) -> glm::vec2 {
                                               const auto a = i * 2.0f * glm::pi<float>() / kCircleMeshVertexCount;
                                               const auto x = kRadius * glm::cos(a);
                                               const auto y = kRadius * glm::sin(a);
                                               return {x, y};
                                           })
                                         | std::ranges::to<std::vector>();
    // clang-format on

    auto mesh = std::make_unique<Mesh>();
    mesh->setVertexData(std::as_bytes(std::span{verts}), verts.size());

    const std::array<Mesh::VertexAttribute, 1> attributes = {Mesh::VertexAttribute{2, Mesh::Type::Float, 0}};
    mesh->setVertexAttributes(attributes, sizeof(glm::vec2));

    return mesh;
}

} // namespace

// TODO: Painter singleton
UniverseMap::UniverseMap(const Universe *universe, Painter *overlayPainter)
    : m_universe(universe)
    , m_overlayPainter(overlayPainter)
{
    initializeMeshes();
}

UniverseMap::~UniverseMap() = default;

void UniverseMap::setViewportSize(const SizeI &size)
{
    m_viewportSize = size;
    m_projectionMatrix =
        glm::perspective(glm::radians(45.0f), static_cast<float>(size.width()) / size.height(), 0.1f, 100.0f);
    m_cameraController.setViewportSize(size);
}

void UniverseMap::render() const
{
    const auto viewMatrix = m_cameraController.viewMatrix();

    glEnable(GL_DEPTH_TEST);

    auto *shaderManager = System::instance()->shaderManager();

    // planet orbits

    shaderManager->setCurrent(ShaderManager::Shader::Orbit);
    shaderManager->setUniform(ShaderManager::Uniform::AspectRatio,
                              static_cast<float>(m_viewportSize.width()) / static_cast<float>(m_viewportSize.height()));
    shaderManager->setUniform(ShaderManager::Uniform::Thickness, 3.0f / static_cast<float>(m_viewportSize.height()));
    shaderManager->setUniform(ShaderManager::Uniform::Color, glm::vec4{0.5, 0.5, 0.5, 1.0});
    const auto worlds = m_universe->worlds();
    for (const auto *world : worlds)
    {
        const auto &orbit = world->orbit();

        const auto elems = orbit.elements();
        const auto semiMajorAxis = elems.semiMajorAxis;
        const auto eccentricity = elems.eccentricity;

        const auto orbitRotation = glm::mat4{orbit.orbitRotationMatrix()};
        const auto mvp = m_projectionMatrix * viewMatrix * orbitRotation;

        shaderManager->setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, mvp);
        shaderManager->setUniform(ShaderManager::Uniform::SemiMajorAxis, semiMajorAxis);
        shaderManager->setUniform(ShaderManager::Uniform::Eccentricity, eccentricity);

#if !defined(ORBIT_WIREFRAME)
        m_orbitMesh->draw(Mesh::Primitive::TriangleStrip);
#else
        m_orbitMesh->draw(Mesh::Primitive::Lines);
#endif
    }

    // ship orbits

    shaderManager->setCurrent(ShaderManager::Shader::PartialOrbit);
    shaderManager->setUniform(ShaderManager::Uniform::AspectRatio,
                              static_cast<float>(m_viewportSize.width()) / static_cast<float>(m_viewportSize.height()));
    shaderManager->setUniform(ShaderManager::Uniform::Thickness, 3.0f / static_cast<float>(m_viewportSize.height()));
    shaderManager->setUniform(ShaderManager::Uniform::Color, glm::vec4{1.0, 0.0, 0.0, 1.0});

    const auto ships = m_universe->ships();
    for (const auto *ship : ships)
    {
        if (auto *orbit = ship->orbit())
        {
            const auto &plan = ship->missionPlan();
            assert(plan.has_value());

            const auto startAngle = orbit->eccentricAnomaly(plan->departureTime);
            const auto currentAngle = orbit->eccentricAnomaly(m_universe->date());
            const auto endAngle = orbit->eccentricAnomaly(plan->arrivalTime);

            shaderManager->setUniform(ShaderManager::Uniform::Thickness,
                                      3.0f / static_cast<float>(m_viewportSize.height()));
            // drawOrbit(*orbit, startAngle, endAngle, glm::vec4{1.0, 0.0, 0.0, 1.0});

            const auto elems = orbit->elements();
            const auto semiMajorAxis = elems.semiMajorAxis;
            const auto eccentricity = elems.eccentricity;

            const auto orbitRotation = glm::mat4{orbit->orbitRotationMatrix()};
            const auto mvp = m_projectionMatrix * viewMatrix * orbitRotation;

            shaderManager->setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, mvp);
            shaderManager->setUniform(ShaderManager::Uniform::SemiMajorAxis, semiMajorAxis);
            shaderManager->setUniform(ShaderManager::Uniform::Eccentricity, eccentricity);
            shaderManager->setUniform(ShaderManager::Uniform::StartAngle, startAngle);
            shaderManager->setUniform(ShaderManager::Uniform::CurrentAngle, currentAngle);
            shaderManager->setUniform(ShaderManager::Uniform::EndAngle, endAngle);

#if !defined(ORBIT_WIREFRAME)
            m_orbitMesh->draw(Mesh::Primitive::TriangleStrip);
#else
            m_orbitMesh->draw(Mesh::Primitive::Lines);
#endif
        }
    }

    // sun mesh

    shaderManager->setCurrent(ShaderManager::Shader::Wireframe);
    {
        shaderManager->setUniform(ShaderManager::Uniform::Color, glm::vec4{1.0, 1.0, 0.5, 1.0f});
        const auto scale = glm::scale(glm::mat4{1.0f}, glm::vec3{0.1f});
        shaderManager->setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix,
                                  m_projectionMatrix * viewMatrix * scale);
#if !defined(SPHERE_WIREFRAME)
        m_sphereMesh->draw(Mesh::Primitive::Triangles);
#else
        m_sphereMesh->draw(Mesh::Primitive::Lines);
#endif
    }

    // planet meshes

    auto drawBodyMesh = [this, shaderManager, &viewMatrix](const Orbit &orbit, float radius) {
        const auto position = orbit.positionOnOrbitPlane(m_universe->date());
        const auto orbitRotation = glm::mat4{orbit.orbitRotationMatrix()};
        const auto translationMatrix = glm::translate(glm::mat4{1.0f}, glm::vec3{position, 0.0f});
        const auto scaleMatrix = glm::scale(glm::mat4{1.0f}, glm::vec3{radius});
        const auto modelMatrix = orbitRotation * translationMatrix * scaleMatrix;
        const auto mvp = m_projectionMatrix * viewMatrix * modelMatrix;
        shaderManager->setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, mvp);
#if !defined(SPHERE_WIREFRAME)
        m_sphereMesh->draw(Mesh::Primitive::Triangles);
#else
        m_sphereMesh->draw(Mesh::Primitive::Lines);
#endif
    };

    shaderManager->setUniform(ShaderManager::Uniform::Color, glm::vec4{0.25, 0.75, 0.25, 1.0f});
    for (const auto *world : worlds)
    {
        drawBodyMesh(world->orbit(), scaledRadius(world));
    }

    // ship meshes
    shaderManager->setUniform(ShaderManager::Uniform::Color, glm::vec4{1.0f});
    for (const auto *ship : ships)
    {
        if (const auto *orbit = ship->orbit())
        {
            drawBodyMesh(*orbit, 0.05f);
        }
    }

    glDisable(GL_DEPTH_TEST);

    // draw labels

    const auto &font = g_styleSettings.normalFont;
    m_overlayPainter->setFont(font);

    auto drawLabel = [&](const glm::vec3 &position, std::string_view name) {
        const auto mvp = m_projectionMatrix * viewMatrix;
        const auto positionProjected = mvp * glm::vec4(position, 1.0);
        if (positionProjected.z > 0.0f)
        {
            glm::vec2 labelPosition;
            labelPosition.x = 0.5f * ((positionProjected.x / positionProjected.w) + 1.0) * m_viewportSize.width() + 5.0;
            labelPosition.y =
                (1.0f - 0.5f * ((positionProjected.y / positionProjected.w) + 1.0)) * m_viewportSize.height() -
                font.pixelHeight;

            m_overlayPainter->setColor({1, 1, 1, 1});
            m_overlayPainter->drawText(labelPosition, name);
        }
    };

    // world labels
    shaderManager->setUniform(ShaderManager::Uniform::Color, glm::vec4(1.0, 1.0, 1.0, 1.0));
    for (const auto *world : worlds)
    {
        drawLabel(world->position(), world->name);
    }

    // ship labels
    shaderManager->setUniform(ShaderManager::Uniform::Color, glm::vec4(1.0, 0.0, 0.0, 1.0));
    for (const auto *ship : ships)
    {
        drawLabel(ship->position(), ship->name);
    }
}

void UniverseMap::initializeMeshes()
{
    m_orbitMesh = createOrbitMesh();
    m_circleBillboardMesh = createBodyBillboardMesh();
    m_sphereMesh = createSphereMesh();
}

void UniverseMap::handleMouseButton(MouseButton button, MouseAction action, const glm::vec2 &pos, Modifier mods)
{
    if (action == MouseAction::Press && button == MouseButton::Left)
    {
        if (const auto *world = pickWorld(pos))
        {
            std::println("picked {}", world->name);
            m_cameraTarget = world;
            m_cameraController.moveCameraCenter(m_cameraTarget->position(), true);
        }
    }
    m_cameraController.handleMouseButton(button, action, pos, mods);
}

void UniverseMap::handleMouseWheel(const glm::vec2 &mousePos, const glm::vec2 &wheelOffset)
{
    m_cameraController.handleMouseWheel(mousePos, wheelOffset);
}

float UniverseMap::scaledRadius(const World *world) const
{
    return 0.05 + 0.04 * std::log(std::max(0.001 * world->radius, 1.0));
}

const World *UniverseMap::pickWorld(const glm::vec2 &viewportPos)
{
    const auto viewMatrix = m_cameraController.viewMatrix();
    auto normalizedPos =
        (viewportPos / glm::vec2{m_viewportSize.width(), m_viewportSize.height()}) * 2.0f - glm::vec2{1.0f};
    normalizedPos.y = -normalizedPos.y;
    const auto viewToWorldMatrix = glm::inverse(m_projectionMatrix * viewMatrix);
    const auto near = viewToWorldMatrix * glm::vec4{normalizedPos, -1.0f, 1.0f};
    const auto far = viewToWorldMatrix * glm::vec4{normalizedPos, 1.0f, 1.0f};

    const auto rayFrom = glm::vec3{near} / near.w;
    const auto rayTo = glm::vec3{far} / far.w;
    const auto rayDir = glm::normalize(rayTo - rayFrom);

    const auto worlds = m_universe->worlds();
    float closestDist = std::numeric_limits<float>::max();
    const World *closestWorld = nullptr;
    for (const auto *world : worlds)
    {
        const auto position = world->position();
        const auto dist = raySphereIntersect(rayFrom, rayDir, position, scaledRadius(world));
        if (dist > 0.0f && dist < closestDist)
        {
            closestDist = dist;
            closestWorld = world;
        }
    }

    return closestWorld;
}

void UniverseMap::handleMouseMove(const glm::vec2 &pos)
{
    m_cameraController.handleMouseMove(pos);
}

void UniverseMap::update(Seconds seconds)
{
    if (m_cameraTarget)
        m_cameraController.moveCameraCenter(m_cameraTarget->position(), true);
    m_cameraController.update(seconds);
}
