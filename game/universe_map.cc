#include "universe_map.h"

#include "shader_manager.h"
#include "painter.h"
#include "mesh.h"

#include <glm/gtx/string_cast.hpp>

namespace
{

static constexpr auto kOrbitVertexCount = 300;
static constexpr auto kCircleMeshVertexCount = 20;

std::unique_ptr<Mesh> createOrbitMesh(const Orbit &orbit)
{
    const auto elems = orbit.elements();
    const auto rotationMatrix = orbit.orbitRotationMatrix();

    const auto semiMajorAxis = elems.semiMajorAxis;
    const auto semiMinorAxis = semiMajorAxis * std::sqrt(1.0 - elems.eccentricity * elems.eccentricity);
    const auto focus = std::sqrt(semiMajorAxis * semiMajorAxis - semiMinorAxis * semiMinorAxis);

    struct Vertex
    {
        glm::vec3 position;
    };
    // clang-format off
    const std::vector<Vertex> verts =
        std::views::iota(0, kOrbitVertexCount)
        | std::views::transform([&rotationMatrix, semiMajorAxis, semiMinorAxis, focus](const std::size_t i) -> Vertex {
              const auto t = i * 2.0f * glm::pi<float>() / kOrbitVertexCount;
              const auto x = semiMajorAxis * glm::cos(t) - focus;
              const auto y = semiMinorAxis * glm::sin(t);
              const auto position = rotationMatrix * glm::vec3(x, y, 0.0f);
              return Vertex{.position = position};
          })
        | std::ranges::to<std::vector>();
    // clang-format on

    auto mesh = std::make_unique<Mesh>();
    mesh->setVertexData(std::as_bytes(std::span{verts}));

    const std::array<Mesh::VertexAttribute, 1> attributes = {
        Mesh::VertexAttribute{3, Mesh::Type::Float, offsetof(Vertex, position)},
    };
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
UniverseMap::UniverseMap(const Universe *universe, ShaderManager *shaderManager, Painter *overlayPainter)
    : m_universe(universe)
    , m_shaderManager(shaderManager)
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

void UniverseMap::render(JulianDate when) const
{
    const auto viewMatrix = m_cameraController.viewMatrix();
    const auto modelMatrix = glm::mat4{1.0f};
    const auto mvp = m_projectionMatrix * viewMatrix * modelMatrix;

    m_shaderManager->setCurrent(ShaderManager::Shader::Wireframe);
    m_shaderManager->setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, mvp);
    m_shaderManager->setUniform(ShaderManager::Uniform::Color, glm::vec4(0.5, 0.5, 0.5, 1.0));

    // render orbits

    const auto worlds = m_universe->worlds();
    const auto ships = m_universe->ships();

    for (const auto *world : worlds)
    {
        auto it = m_orbitMeshes.find(world);
        assert(it != m_orbitMeshes.end());
        auto &orbitMesh = it->second;
        orbitMesh->draw(Mesh::Primitive::LineLoop, 0, kOrbitVertexCount);
    }

    for (const auto *ship : ships)
    {
        const auto transit = ship->transit();
        if (transit.has_value() && transit->departureTime < when && when < transit->arrivalTime)
        {
            // TODO draw transit orbit
        }
    }

    const Font font{"DejaVuSans.ttf", 20.0f, 0};
    m_overlayPainter->setFont(font);

    m_shaderManager->setCurrent(ShaderManager::Shader::Billboard);
    m_shaderManager->setUniform(ShaderManager::Uniform::ProjectionMatrix, m_projectionMatrix);
    m_shaderManager->setUniform(ShaderManager::Uniform::ViewMatrix, viewMatrix);

    // render sun billboard

    m_shaderManager->setUniform(ShaderManager::Uniform::Color, glm::vec4(1.0, 1.0, 0.0, 1.0));
    m_shaderManager->setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, mvp);
    m_shaderManager->setUniform(ShaderManager::Uniform::ModelMatrix, modelMatrix);
    m_bodyBillboardMesh->draw(Mesh::Primitive::LineLoop, 0, kCircleMeshVertexCount);

    auto drawBillboard = [&](const glm::vec3 &position, std::string_view name) {
        // billboard

        const auto bodyModelMatrix = modelMatrix * glm::translate(glm::mat4(1.0), position);
        const auto mvp = m_projectionMatrix * viewMatrix * bodyModelMatrix;
        m_shaderManager->setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, mvp);
        m_shaderManager->setUniform(ShaderManager::Uniform::ModelMatrix, bodyModelMatrix);
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

            m_overlayPainter->drawText(labelPosition, name);
        }
    };

    // render world billboards

    m_shaderManager->setUniform(ShaderManager::Uniform::Color, glm::vec4(1.0, 1.0, 1.0, 1.0));
    for (const auto *world : worlds)
    {
        drawBillboard(world->position(when), world->name());
    }

    // render ship billboards

    m_shaderManager->setUniform(ShaderManager::Uniform::Color, glm::vec4(1.0, 0.0, 0.0, 1.0));
    for (const auto *ship : ships)
    {
        const auto transit = ship->transit();
        if (transit.has_value() && transit->departureTime < when && when < transit->arrivalTime)
        {
            drawBillboard(transit->orbit.position(when), ship->name());
        }
    }
}

void UniverseMap::initializeMeshes()
{
    const auto worlds = m_universe->worlds();
    for (const auto *world : worlds)
    {
        auto orbitMesh = createOrbitMesh(world->orbit());
        m_orbitMeshes[world] = std::move(orbitMesh);
    }

    m_bodyBillboardMesh = createBodyBillboardMesh();
}

void UniverseMap::handleMouseButton(MouseButton button, MouseAction action, const glm::vec2 &position, Modifier mods)
{
    m_cameraController.handleMouseButton(button, action, position, mods);
}

void UniverseMap::handleMouseMove(const glm::vec2 &position)
{
    m_cameraController.handleMouseMove(position);
}

void UniverseMap::update(Seconds seconds)
{
    m_cameraController.update(seconds);
}
