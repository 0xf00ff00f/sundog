#include "universe_map.h"

#include "style_settings.h"

#include <base/system.h>
#include <base/shader_manager.h>
#include <base/painter.h>
#include <base/mesh.h>

#include <glm/gtx/string_cast.hpp>

// #define ORBIT_WIREFRAME

namespace
{

static constexpr auto kOrbitVertexCount = 300;
static constexpr auto kCircleMeshVertexCount = 20;

std::unique_ptr<Mesh> createOrbitMesh()
{
    struct Vertex
    {
        float meanAnomaly;
        float normalDirection;
    };

    auto mesh = std::make_unique<Mesh>();

    std::vector<Vertex> verts;
    verts.reserve(kOrbitVertexCount * 2);
    for (std::size_t i = 0; i < kOrbitVertexCount; ++i)
    {
        const auto meanAnomaly = i * 2.0f * glm::pi<float>() / (kOrbitVertexCount - 1);
        verts.emplace_back(meanAnomaly, -1.0f);
        verts.emplace_back(meanAnomaly, 1.0f);
    }
    mesh->setVertexData(std::as_bytes(std::span{verts}));

#if defined(ORBIT_WIREFRAME)
    std::vector<uint32_t> indices;
    indices.reserve(kOrbitVertexCount * 8);
    for (std::size_t i = 0; i < kOrbitVertexCount; ++i)
    {
        const auto baseIndex = i * 2;

        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 1);

        indices.push_back(baseIndex);
        indices.push_back((baseIndex + 2) % (kOrbitVertexCount * 2));

        indices.push_back(baseIndex + 1);
        indices.push_back((baseIndex + 2) % (kOrbitVertexCount * 2));

        indices.push_back(baseIndex + 1);
        indices.push_back((baseIndex + 3) % (kOrbitVertexCount * 2));
    }
    mesh->setIndexData(std::span{indices});
#endif

    const std::array<Mesh::VertexAttribute, 2> attributes = {
        Mesh::VertexAttribute{1, Mesh::Type::Float, offsetof(Vertex, meanAnomaly)},
        Mesh::VertexAttribute{1, Mesh::Type::Float, offsetof(Vertex, normalDirection)}};
    mesh->setVertexAttributes(attributes, sizeof(Vertex));

    return mesh;
}

std::unique_ptr<Mesh> createBodyBillboardMesh()
{
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
    mesh->setVertexData(std::as_bytes(std::span{verts}));

    const std::array<Mesh::VertexAttribute, 1> attributes = {Mesh::VertexAttribute{2, Mesh::Type::Float, 0}};
    mesh->setVertexAttributes(attributes, sizeof(glm::vec2));

    return mesh;
}

} // namespace

// TODO: ShaderManager, Painter singleton
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

    // render orbits

    auto *shaderManager = System::instance()->shaderManager();
    shaderManager->setCurrent(ShaderManager::Shader::Orbit);
    shaderManager->setUniform(ShaderManager::Uniform::AspectRatio,
                              static_cast<float>(m_viewportSize.width()) / static_cast<float>(m_viewportSize.height()));
    shaderManager->setUniform(ShaderManager::Uniform::Thickness, 3.0f / static_cast<float>(m_viewportSize.height()));
    auto drawOrbit = [this, shaderManager, &viewMatrix](const Orbit &orbit, const glm::vec4 &color) {
        const auto elems = orbit.elements();
        const auto rotationMatrix = orbit.orbitRotationMatrix();

        const auto semiMajorAxis = elems.semiMajorAxis;
        const auto eccentricity = elems.eccentricity;

        const auto modelMatrix = glm::mat4{orbit.orbitRotationMatrix()};
        const auto mvp = m_projectionMatrix * viewMatrix * modelMatrix;

        shaderManager->setUniform(ShaderManager::Uniform::Color, color);
        shaderManager->setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, mvp);
        shaderManager->setUniform(ShaderManager::Uniform::SemiMajorAxis, semiMajorAxis);
        shaderManager->setUniform(ShaderManager::Uniform::Eccentricity, eccentricity);

#if !defined(ORBIT_WIREFRAME)
        m_orbitMesh->draw(Mesh::Primitive::TriangleStrip, 0, 2 * kOrbitVertexCount);
#else
        m_orbitMesh->drawElements(Mesh::Primitive::Lines, kOrbitVertexCount * 8);
#endif
    };

    const auto worlds = m_universe->worlds();
    for (const auto *world : worlds)
    {
        drawOrbit(world->orbit(), glm::vec4{0.5, 0.5, 0.5, 1.0});
    }

    const auto ships = m_universe->ships();
    for (const auto *ship : ships)
    {
        if (auto *orbit = ship->orbit())
        {
            drawOrbit(*orbit, glm::vec4{1.0, 0.0, 0.0, 1.0});
        }
    }

    const auto modelMatrix = glm::mat4{1.0f};
    const auto mvp = m_projectionMatrix * viewMatrix * modelMatrix;

    const auto &font = g_styleSettings.normalFont;
    m_overlayPainter->setFont(font);

    shaderManager->setCurrent(ShaderManager::Shader::Billboard);
    shaderManager->setUniform(ShaderManager::Uniform::ProjectionMatrix, m_projectionMatrix);
    shaderManager->setUniform(ShaderManager::Uniform::ViewMatrix, viewMatrix);

    // render sun billboard

    shaderManager->setUniform(ShaderManager::Uniform::Color, glm::vec4(1.0, 1.0, 0.0, 1.0));
    shaderManager->setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, mvp);
    shaderManager->setUniform(ShaderManager::Uniform::ModelMatrix, modelMatrix);
    m_bodyBillboardMesh->draw(Mesh::Primitive::LineLoop, 0, kCircleMeshVertexCount);

    auto drawBillboard = [&](const glm::vec3 &position, std::string_view name) {
        // billboard

        const auto bodyModelMatrix = modelMatrix * glm::translate(glm::mat4(1.0), position);
        const auto mvp = m_projectionMatrix * viewMatrix * bodyModelMatrix;
        shaderManager->setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, mvp);
        shaderManager->setUniform(ShaderManager::Uniform::ModelMatrix, bodyModelMatrix);
        m_bodyBillboardMesh->draw(Mesh::Primitive::LineLoop, 0, kCircleMeshVertexCount);

        // label

        const auto positionProjected = mvp * glm::vec4(0.0, 0.0, 0.0, 1.0);
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

    // render world billboards

    shaderManager->setUniform(ShaderManager::Uniform::Color, glm::vec4(1.0, 1.0, 1.0, 1.0));
    for (const auto *world : worlds)
    {
        drawBillboard(world->position(), world->name());
    }

    // render ship billboards

    shaderManager->setUniform(ShaderManager::Uniform::Color, glm::vec4(1.0, 0.0, 0.0, 1.0));
    for (const auto *ship : ships)
    {
        drawBillboard(ship->position(), ship->name());
    }
}

void UniverseMap::initializeMeshes()
{
    m_orbitMesh = createOrbitMesh();
    m_bodyBillboardMesh = createBodyBillboardMesh();
}

void UniverseMap::handleMouseButton(MouseButton button, MouseAction action, const glm::vec2 &pos, Modifier mods)
{
    m_cameraController.handleMouseButton(button, action, pos, mods);
}

void UniverseMap::handleMouseMove(const glm::vec2 &pos)
{
    m_cameraController.handleMouseMove(pos);
}

void UniverseMap::update(Seconds seconds)
{
    m_cameraController.update(seconds);
}
