#include "universe_map.h"

#include "style_settings.h"

#include <base/system.h>
#include <base/shader_manager.h>
#include <base/painter.h>
#include <base/mesh.h>
#include <base/gui.h>
#include <base/texture_cache.h>

#include <glm/gtx/string_cast.hpp>

// #define ORBIT_WIREFRAME
// #define SPHERE_WIREFRAME

namespace
{

double scaledRadius(double radius)
{
    return 0.05 + 0.04 * std::log(std::max(0.001 * radius, 1.0));
}

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
    constexpr auto kRings = 30;
    constexpr auto kSlices = 30;

    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
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
            const auto lon = j * 2.0f * glm::pi<float>() / (kSlices - 1) - glm::pi<float>();
            const auto v = static_cast<float>(j) / (kSlices - 1);

            const auto position = latLonToCartesian(lat, lon);
            const auto normal = glm::normalize(position);
            const auto texCoord = glm::vec2{v, u};

            verts.emplace_back(position, normal, texCoord);
        }
    }
    mesh->setVertexData(std::as_bytes(std::span{verts}), verts.size());

    std::vector<uint32_t> indices;
    for (std::size_t i = 0; i < kRings - 1; ++i)
    {
        for (std::size_t j = 0; j < kSlices - 1; ++j)
        {
            const auto v0 = i * kSlices + j;
            const auto v1 = (i + 1) * kSlices + j;
            const auto v2 = (i + 1) * kSlices + j + 1;
            const auto v3 = i * kSlices + j + 1;

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

    const std::array<Mesh::VertexAttribute, 3> attributes = {
        Mesh::VertexAttribute{3, Mesh::Type::Float, offsetof(Vertex, position)},
        Mesh::VertexAttribute{3, Mesh::Type::Float, offsetof(Vertex, normal)},
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

class MapLabel : public ui::Column
{
public:
    explicit MapLabel(ui::Gizmo *parent = nullptr);

    virtual const World *world() const { return nullptr; }
    virtual const Ship *ship() const { return nullptr; }
    // TODO rename to clipSpacePosition
    virtual glm::vec3 clipSpacePosition(const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix) const = 0;
    void paintContents(Painter *painter, const glm::vec2 &pos, int depth) const override;
    virtual bool visible() const = 0;
};

class WorldLabel : public MapLabel
{
public:
    explicit WorldLabel(const World *world);

    const World *world() const override { return m_world; }
    glm::vec3 clipSpacePosition(const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix) const override;
    bool visible() const override { return true; }

private:
    const World *m_world{nullptr};
    ui::Text *m_text{nullptr};
};

class ShipLabel : public MapLabel
{
public:
    explicit ShipLabel(const Ship *ship);

    const Ship *ship() const override { return m_ship; }
    glm::vec3 clipSpacePosition(const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix) const override;
    bool visible() const override { return m_ship->state() == Ship::State::InTransit; }

private:
    void updateStateText();

    const Ship *m_ship{nullptr};
    ui::Text *m_text{nullptr};
    ui::MultiLineText *m_stateText{nullptr};
    ui::MultiLineText *m_etaText{nullptr};
    ui::MultiLineText *m_speedText{nullptr};
    ui::Text *m_speed{nullptr};
    muslots::Connection m_stateChangedConnection;
};

MapLabel::MapLabel(ui::Gizmo *parent)
    : ui::Column(parent)
{
    setMargins(8.0f, 8.0f, 8.0f, 14.0f);
}

void MapLabel::paintContents(Painter *painter, const glm::vec2 &pos, int depth) const
{
    constexpr auto kThickness = 1.0f;
    constexpr auto kArrowHeight = 6.0f;
    const auto rect = RectF{pos, SizeF{m_size.width(), m_size.height() - kArrowHeight}};
    painter->setColor(glm::vec4{0.0f, 0.0f, 0.0f, 0.75f});
    painter->fillRoundedRect(rect, 4.0f, depth);
    const std::array<glm::vec2, 3> arrow{glm::vec2{rect.left() + 0.5f * rect.width() - kArrowHeight, rect.bottom()},
                                         glm::vec2{rect.left() + 0.5f * rect.width(), rect.bottom() + kArrowHeight},
                                         glm::vec2{rect.left() + 0.5f * rect.width() + kArrowHeight, rect.bottom()}};
    painter->fillConvexPolygon(arrow, depth);
}

WorldLabel::WorldLabel(const World *world)
    : MapLabel()
    , m_world(world)
{
    m_text = appendChild<ui::Text>();
    m_text->setFont(g_styleSettings.smallFont);
    m_text->color = g_styleSettings.accentColor;
    m_text->setText(m_world->name);
}

glm::vec3 WorldLabel::clipSpacePosition(const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix) const
{
    auto viewPosition = viewMatrix * glm::vec4{m_world->position(), 1.0f};
    viewPosition.y += scaledRadius(m_world->radius); // move it to top of the planet
    const auto clipSpacePosition = projectionMatrix * viewPosition;
    return glm::vec3{clipSpacePosition} / clipSpacePosition.w;
}

ShipLabel::ShipLabel(const Ship *ship)
    : MapLabel()
    , m_ship(ship)
    , m_stateChangedConnection(m_ship->stateChangedSignal.connect([this](Ship::State) { updateStateText(); }))
{
    m_text = appendChild<ui::Text>();
    m_text->setFont(g_styleSettings.smallFont);
    m_text->color = g_styleSettings.accentColor;
    m_text->setText(ship->shipClass()->name);

    auto *separator = appendChild<ui::Rectangle>(120.0f, 1.0f);
    separator->setFillBackground(1);
    separator->backgroundColor = glm::vec4{1.0f, 1.0f, 1.0f, 0.5f};

    m_stateText = appendChild<ui::MultiLineText>();
    m_stateText->setFont(g_styleSettings.smallFont);
    m_stateText->color = g_styleSettings.baseColor;
    m_stateText->setLineWidth(separator->width());

    m_etaText = appendChild<ui::MultiLineText>();
    m_etaText->setFont(g_styleSettings.smallFont);
    m_etaText->color = g_styleSettings.baseColor;
    m_etaText->setLineWidth(separator->width());

    m_speedText = appendChild<ui::MultiLineText>();
    m_speedText->setFont(g_styleSettings.smallFont);
    m_speedText->color = g_styleSettings.baseColor;
    m_speedText->setLineWidth(separator->width());

    updateStateText();
}

void ShipLabel::updateStateText()
{
    switch (m_ship->state())
    {
    case Ship::State::InTransit: {
        const auto &missionPlan = m_ship->missionPlan();
        assert(missionPlan.has_value());
        m_stateText->setText(std::format("{} > {}", missionPlan->origin->name, missionPlan->destination->name));
        const auto eta = missionPlan->arrivalTime;
        m_etaText->setText(std::format("ETA {:D}", eta));

        const auto *orbit = m_ship->orbit();
        assert(orbit != nullptr);
        const auto [_, velocity] = orbit->stateVector(m_ship->universe()->date());
        const auto speed = glm::length(velocity) * 1.496e+8 / (24 * 60 * 60);
        m_speedText->setText(std::format("{:.2f} km/s", speed));
        break;
    }
    case Ship::State::Docked: {
        const auto *world = m_ship->world();
        assert(world);
        m_stateText->setText(std::format("Docked on {}", world->name));
        break;
    }
    }
}

glm::vec3 ShipLabel::clipSpacePosition(const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix) const
{
    const auto viewProjectionMatrix = projectionMatrix * viewMatrix;
    const auto clipSpacePosition = viewProjectionMatrix * glm::vec4{m_ship->position(), 1.0f};
    return glm::vec3{clipSpacePosition} / clipSpacePosition.w;
}

// TODO: Painter singleton
UniverseMap::UniverseMap(Universe *universe, Painter *overlayPainter)
    : m_universe(universe)
    , m_overlayPainter(overlayPainter)
{
    initializeMeshes();
    initializeLabels();

    m_connections.emplace_back(m_universe->shipAddedSignal.connect(
        [this](const Ship *ship) { m_labels.emplace_back(std::make_unique<ShipLabel>(ship)); }));
    m_connections.emplace_back(m_universe->shipAboutToBeRemovedSignal.connect([this](const Ship *ship) {
        std::erase_if(m_labels, [ship](const auto &label) { return label->ship() == ship; });
    }));
}

UniverseMap::~UniverseMap()
{
    for (auto &connection : m_connections)
        connection.disconnect();
}

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
    auto *textureCache = System::instance()->textureCache();

    // planet orbits

    shaderManager->setCurrent(ShaderManager::Shader::Orbit);
    shaderManager->setUniform(ShaderManager::Uniform::AspectRatio,
                              static_cast<float>(m_viewportSize.width()) / static_cast<float>(m_viewportSize.height()));
    shaderManager->setUniform(ShaderManager::Uniform::Thickness, 1.0f / static_cast<float>(m_viewportSize.height()));
    shaderManager->setUniform(ShaderManager::Uniform::Color, glm::vec4{0.75, 0.75, 0.75, 1.0});
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

    {
        // TODO this looks like garbage

        shaderManager->setCurrent(ShaderManager::Shader::Wireframe);
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

    shaderManager->setCurrent(ShaderManager::Shader::Planet);
    shaderManager->setUniform(ShaderManager::Uniform::LightPosition, glm::vec3{0.0});
    shaderManager->setUniform(ShaderManager::Uniform::LightIntensity, glm::vec3{1.0});
    shaderManager->setUniform(ShaderManager::Uniform::Ambient, glm::vec3{0.1});
    shaderManager->setUniform(ShaderManager::Uniform::Specular, glm::vec3{0.1});
    shaderManager->setUniform(ShaderManager::Uniform::Shininess, 50.0);
    for (const auto *world : worlds)
    {
        constexpr auto kTiltAxis = glm::vec3{0.0, 1.0, 0.0};
        constexpr auto kRollAxis = glm::vec3{0.0, 0.0, 1.0};

        const float tilt = world->axialTilt;

        const auto t = m_universe->date().time_since_epoch().count(); // XXX not really
        double dummy;
        const float alpha = std::modf(t / world->rotationPeriod.count(), &dummy);
        const float roll = alpha * 2.0 * glm::pi<float>();

        const auto radius = static_cast<float>(scaledRadius(world->radius));

        const auto *texture = textureCache->findOrCreateTexture(world->diffuseTexture);
        texture->bind();

        const auto &orbit = world->orbit();
        const auto position = orbit.positionOnOrbitPlane(m_universe->date());
        const auto orbitRotation = glm::mat4{orbit.orbitRotationMatrix()};
        const auto translationMatrix = glm::translate(glm::mat4{1.0f}, glm::vec3{position, 0.0f});
        const auto tiltRotationMatrix = glm::rotate(glm::mat4{1.0f}, tilt, kTiltAxis);
        const auto rollRotationMatrix = glm::rotate(glm::mat4{1.0f}, roll, kRollAxis);
        const auto scaleMatrix = glm::scale(glm::mat4{1.0f}, glm::vec3{radius});
        const auto modelMatrix =
            orbitRotation * translationMatrix * tiltRotationMatrix * rollRotationMatrix * scaleMatrix;
        const auto modelViewMatrix = viewMatrix * modelMatrix;
        shaderManager->setUniform(ShaderManager::Uniform::ViewMatrix, viewMatrix);
        shaderManager->setUniform(ShaderManager::Uniform::ModelViewMatrix, modelViewMatrix);
        shaderManager->setUniform(ShaderManager::Uniform::ModelViewNormalMatrix,
                                  glm::transpose(glm::inverse(glm::mat3{modelViewMatrix})));
        const auto mvp = m_projectionMatrix * modelViewMatrix;
        shaderManager->setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, mvp);
#if !defined(SPHERE_WIREFRAME)
        m_sphereMesh->draw(Mesh::Primitive::Triangles);
#else
        m_sphereMesh->draw(Mesh::Primitive::Lines);
#endif
    }

    // ship meshes
    shaderManager->setCurrent(ShaderManager::Shader::Wireframe);
    shaderManager->setUniform(ShaderManager::Uniform::Color, glm::vec4{1.0});
    for (const auto *ship : ships)
    {
        if (const auto *orbit = ship->orbit())
        {
            constexpr auto kRadius = 0.025f;
            const auto position = orbit->positionOnOrbitPlane(m_universe->date());
            const auto orbitRotation = glm::mat4{orbit->orbitRotationMatrix()};
            const auto translationMatrix = glm::translate(glm::mat4{1.0f}, glm::vec3{position, 0.0f});
            const auto scaleMatrix = glm::scale(glm::mat4{1.0f}, glm::vec3{kRadius});
            const auto modelMatrix = orbitRotation * translationMatrix * scaleMatrix;
            const auto mvp = m_projectionMatrix * viewMatrix * modelMatrix;
            shaderManager->setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, mvp);
#if !defined(SPHERE_WIREFRAME)
            m_sphereMesh->draw(Mesh::Primitive::Triangles);
#else
            m_sphereMesh->draw(Mesh::Primitive::Lines);
#endif
        }
    }

    glDisable(GL_DEPTH_TEST);

    // draw labels

    // clang-format off
    auto labelPositions =
        m_labels
        | std::views::filter(&MapLabel::visible)
        | std::views::transform([this, &viewMatrix](const auto &label) {
              return std::make_pair(label.get(), label->clipSpacePosition(m_projectionMatrix, viewMatrix));
          })
        | std::views::filter([](const auto &item) {
              const auto &[_, clipSpacePosition] = item;
              return clipSpacePosition.z > -1.0f && clipSpacePosition.z < 1.0f;
          })
        | std::ranges::to<std::vector>();
    // clang-format on

    // back to front
    std::ranges::stable_sort(labelPositions, std::greater{}, [](const auto &item) { return item.second.z; });

    for (std::size_t depth = 0; const auto &[label, positionProjected] : labelPositions)
    {
        glm::vec2 screenPosition = (glm::vec2{positionProjected} * glm::vec2{0.5f, -0.5f} + glm::vec2{0.5f}) *
                                   glm::vec2{m_viewportSize.width(), m_viewportSize.height()};
        screenPosition -= glm::vec2{0.5f * label->width(), label->height()};
        label->paint(m_overlayPainter, screenPosition, depth);
        depth += 10;
    }
}

void UniverseMap::initializeMeshes()
{
    m_orbitMesh = createOrbitMesh();
    m_circleBillboardMesh = createBodyBillboardMesh();
    m_sphereMesh = createSphereMesh();
}

void UniverseMap::initializeLabels()
{
    for (const auto *world : m_universe->worlds())
        m_labels.emplace_back(std::make_unique<WorldLabel>(world));

    for (const auto *ship : m_universe->ships())
        m_labels.emplace_back(std::make_unique<ShipLabel>(ship));
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
        const auto dist =
            raySphereIntersect(rayFrom, rayDir, position, static_cast<float>(scaledRadius(world->radius)));
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
