#include "universe_map.h"

#include "shader_manager.h"
#include "painter.h"
#include "mesh.h"

namespace
{

static constexpr auto kOrbitVertexCount = 300;
static constexpr auto kCircleMeshVertexCount = 20;

std::unique_ptr<Mesh> createOrbitMesh(const Body &body)
{
    const auto orbit = body.orbitalElements();
    const auto rotationMatrix = body.orbitRotationMatrix();

    const auto semiMajorAxis = orbit.semiMajorAxis;
    const auto semiMinorAxis = semiMajorAxis * std::sqrt(1.0 - orbit.eccentricity * orbit.eccentricity);
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

void UniverseMap::setViewport(int width, int height)
{
    m_viewportWidth = width;
    m_viewportHeight = height;
    m_projectionMatrix = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / height, 0.1f, 100.0f);
}

void UniverseMap::render(JulianDate when) const
{
    const auto viewMatrix =
        glm::lookAt(glm::vec3(0.0f, 0.0f, 6.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const auto modelMatrix = glm::mat4(1.0f);
    const auto mvp = m_projectionMatrix * viewMatrix * modelMatrix;

    m_shaderManager->setCurrent(ShaderManager::Shader::Wireframe);
    m_shaderManager->setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, mvp);
    m_shaderManager->setUniform(ShaderManager::Uniform::Color, glm::vec4(0.5, 0.5, 0.5, 1.0));

    // render orbits

    const auto worlds = m_universe->worlds();

    for (const auto &world : worlds)
    {
        auto it = m_orbitMeshes.find(&world);
        assert(it != m_orbitMeshes.end());
        auto &orbitMesh = it->second;
        orbitMesh->draw(Mesh::Primitive::LineLoop, 0, kOrbitVertexCount);
    }

    const Font font{"DejaVuSans.ttf", 20.0f, 0};
    m_overlayPainter->setFont(font);

    m_shaderManager->setCurrent(ShaderManager::Shader::Billboard);
    m_shaderManager->setUniform(ShaderManager::Uniform::ProjectionMatrix, m_projectionMatrix);
    m_shaderManager->setUniform(ShaderManager::Uniform::ViewMatrix, viewMatrix);
    m_shaderManager->setUniform(ShaderManager::Uniform::Color, glm::vec4(0.5, 0.5, 0.5, 1.0));

    // render sun billboard

    m_shaderManager->setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, mvp);
    m_shaderManager->setUniform(ShaderManager::Uniform::ModelMatrix, modelMatrix);
    m_bodyBillboardMesh->draw(Mesh::Primitive::LineLoop, 0, kCircleMeshVertexCount);

    for (auto &world : worlds)
    {
        // render world billboard

        const auto &body = world.body();
        const auto position = body.position(when);
        const auto bodyModelMatrix = modelMatrix * glm::translate(glm::mat4(1.0), position);
        const auto mvp = m_projectionMatrix * viewMatrix * bodyModelMatrix;
        m_shaderManager->setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, mvp);
        m_shaderManager->setUniform(ShaderManager::Uniform::ModelMatrix, bodyModelMatrix);
        m_bodyBillboardMesh->draw(Mesh::Primitive::LineLoop, 0, kCircleMeshVertexCount);

        // render label

        const auto positionProjected = mvp * glm::vec4(0.0, 0.0, 0.0, 1.0);
        glm::vec2 labelPosition;
        labelPosition.x = 0.5f * ((positionProjected.x / positionProjected.w) + 1.0) * m_viewportWidth + 5.0;
        labelPosition.y =
            (1.0f - 0.5f * ((positionProjected.y / positionProjected.w) + 1.0)) * m_viewportHeight - font.pixelHeight;

        m_overlayPainter->drawText(labelPosition, world.name());
    }
}

void UniverseMap::initializeMeshes()
{
    const auto worlds = m_universe->worlds();
    for (const auto &world : worlds)
    {
        auto orbitMesh = createOrbitMesh(world.body());
        m_orbitMeshes[&world] = std::move(orbitMesh);
    }

    m_bodyBillboardMesh = createBodyBillboardMesh();
}
