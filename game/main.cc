#include "glhelpers.h"
#include "mesh.h"
#include "glyph_cache.h"
#include "tile_batcher.h"

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/string_cast.hpp>

#include <nlohmann/json.hpp>

#include <chrono>
#include <fstream>
#include <cassert>
#include <cstdio>

// https://farside.ph.utexas.edu/teaching/celestial/Celestial/node34.html
// http://astro.if.ufrgs.br/trigesf/position.html
// http://www.davidcolarusso.com/astro/

struct JulianClock
{
    using rep = float;
    using period = std::ratio<60 * 60 * 24>;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<JulianClock>;

    static time_point now()
    {
        return time_point{duration{std::chrono::system_clock::now().time_since_epoch()} + duration{2440587.5}};
    }
};

using JulianDate = std::chrono::time_point<JulianClock>;

struct OrbitalElements
{
    JulianDate epoch;
    float semiMajorAxis = 1.0f;
    float eccentricity = 0.0f;
    float inclination = 0.0f;            // radians
    float longitudePerihelion = 0.0f;    // radians
    float longitudeAscendingNode = 0.0f; // radians
    float meanAnomalyAtEpoch = 0.0f;     // radians
};
// longitudePerihelion = longitudeAscendingNode + argumentPerihelion

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
    void initializeOrbitMesh();
    glm::mat3 orbitRotationMatrix() const;

    static constexpr auto kOrbitVertexCount = 300;

    OrbitalElements m_orbit;
    float m_period = 0.0f;
    Mesh m_orbitMesh;
};

Body::Body()
{
    initializeOrbitMesh();
}

void Body::setOrbitalElements(const OrbitalElements &orbit)
{
    constexpr auto kEarthYearInDays = 365.2425;
    m_orbit = orbit;
    m_period = std::pow(m_orbit.semiMajorAxis, 3.0 / 2.0) * kEarthYearInDays;
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
    const auto E = eccentricAnomaly(when);

    const auto a = m_orbit.semiMajorAxis;
    const auto e = m_orbit.eccentricity;

    // position in orbit
    const auto x = a * (std::cos(E) - e);
    const auto y = a * (std::sqrt(1.0 - e * e) * std::sin(E));

    const auto position = orbitRotationMatrix() * glm::vec3(x, y, 0.0);

#if 0
    {
        const auto w = glm::radians(m_orbit.longitudePerihelion - m_orbit.longitudeAscendingNode);
        const auto i = glm::radians(m_orbit.inclination);
        const auto N = glm::radians(m_orbit.longitudeAscendingNode);

        const auto v = std::atan2(y, x);
        const auto r = std::sqrt(x * x + y * y);
        const auto xh = r * (std::cos(N) * std::cos(v + w) - std::sin(N) * std::sin(v + w) * std::cos(i));
        const auto yh = r * (std::sin(N) * std::cos(v + w) + std::cos(N) * std::sin(v + w) * std::cos(i));
        const auto zh = r * std::sin(v + w) * std::sin(i);

        std::println("position={} expected={}", glm::to_string(position), glm::to_string(glm::vec3(xh, yh, zh)));
    }
#endif

    return position;
}

glm::mat3 Body::orbitRotationMatrix() const
{
    const auto w = m_orbit.longitudePerihelion - m_orbit.longitudeAscendingNode;
    const auto rw = glm::mat3(glm::rotate(glm::mat4(1.0), w, glm::vec3(0.0, 0.0, 1.0)));

    const auto i = m_orbit.inclination;
    const auto ri = glm::mat3(glm::rotate(glm::mat4(1.0), i, glm::vec3(1.0, 0.0, 0.0)));

    const auto N = m_orbit.longitudeAscendingNode;
    const auto rN = glm::mat3(glm::rotate(glm::mat4(1.0), N, glm::vec3(0.0, 0.0, 1.0)));

    return rN * ri * rw;
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
        | std::views::transform([semiMajorAxis, semiMinorAxis, focus, rotationMatrix = orbitRotationMatrix()](const std::size_t i) -> Vertex {
              const auto t = i * 2.0f * glm::pi<float>() / kOrbitVertexCount;
              const auto x = semiMajorAxis * glm::cos(t) - focus;
              const auto y = semiMinorAxis * glm::sin(t);
              const auto position = rotationMatrix * glm::vec3(x, y, 0.0f);
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

std::optional<gl::ShaderProgram> initializeShaderProgram(const char *vertexShader, const char *fragmentShader)
{
    gl::ShaderProgram program;
    if (!program.attachShader(gl::ShaderProgram::ShaderType::FragmentShader, fragmentShader))
        return {};
    if (!program.attachShader(gl::ShaderProgram::ShaderType::VertexShader, vertexShader))
        return {};
    if (!program.link())
        return {};
    return program;
}

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

struct Universe
{
public:
    Universe();

    bool load(const nlohmann::json &json);
    std::span<const World> worlds() const;

private:
    std::vector<World> m_worlds;
};

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

std::string assetPath(std::string_view name)
{
    return std::format("{}{}", ASSETSDIR, name);
}

// TODO: shader manager

int main(int argc, char *argv[])
{
    if (!glfwInit())
    {
        std::println(stderr, "Failed to initialize GLFW");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(1200, 600, "Hello", nullptr, nullptr);
    if (!window)
    {
        std::println(stderr, "Failed to create window");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    const auto gladVersion = gladLoadGL(glfwGetProcAddress);
    if (gladVersion == 0)
    {
        std::println(stderr, "Failed to initialize OpenGL context");
        glfwTerminate();
        return 1;
    }
    std::println("Loaded OpenGL {}.{}", GLAD_VERSION_MAJOR(gladVersion), GLAD_VERSION_MINOR(gladVersion));
    std::println("OpenGL renderer: {}", reinterpret_cast<const char *>(glGetString(GL_RENDERER)));
    std::println("OpenGL version: {}", reinterpret_cast<const char *>(glGetString(GL_VERSION)));

    {
        using namespace gl;

        const char *orbitVertexShader = R"(
layout(location=0) in vec3 position;

uniform mat4 mvp;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
}
)";
        const char *billboardVertexShader = R"(
layout(location=0) in vec2 position;

uniform mat4 mvp;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main() {
    vec3 cameraRight = vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
    vec3 cameraUp = vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
    vec3 worldCenter = vec3(modelMatrix * vec4(0.0, 0.0, 0.0, 1.0));
    vec3 worldPosition = worldCenter + cameraRight * position.x + cameraUp * position.y;
    gl_Position = projectionMatrix * viewMatrix * vec4(worldPosition, 1.0);
    // gl_Position = mvp * vec4(position, 0.0, 1.0);
}
)";

        const char *lineFragmentShader = R"(
out vec4 fragColor;

void main() {
    // fragColor = vec4(1.0);
    fragColor = vec4(0.5, 0.5, 0.5, 1.0);
}
)";

        const char *textVertexShader = R"(
layout(location=0) in vec2 position;
layout(location=1) in vec2 texCoord;

uniform mat4 mvp;

out vec2 vs_texCoord;

void main() {
    vs_texCoord = texCoord;
    gl_Position = mvp * vec4(position, 0.0, 1.0);
}
)";

        const char *textFragmentShader = R"(
uniform sampler2D spriteSheetTexture;

in vec2 vs_texCoord;

out vec4 fragColor;

void main() {
    // fragColor = vec4(1.0); // texture(spriteSheetTexture, vs_texCoord);
    // fragColor = vec4(vs_texCoord, 0.0, 1.0);
    fragColor = texture(spriteSheetTexture, vs_texCoord);
}
)";
        auto maybeProgram = initializeShaderProgram(orbitVertexShader, lineFragmentShader);
        if (!maybeProgram.has_value())
        {
            glfwTerminate();
            return 1;
        }
        ShaderProgram orbitProgram = std::move(maybeProgram.value());

        maybeProgram = initializeShaderProgram(billboardVertexShader, lineFragmentShader);
        if (!maybeProgram.has_value())
        {
            glfwTerminate();
            return 1;
        }
        ShaderProgram billboardProgram = std::move(maybeProgram.value());

        maybeProgram = initializeShaderProgram(textVertexShader, textFragmentShader);
        if (!maybeProgram.has_value())
        {
            glfwTerminate();
            return 1;
        }
        ShaderProgram textProgram = std::move(maybeProgram.value());

        std::ifstream f(assetPath("universe.json"));
        const nlohmann::json universeJson = nlohmann::json::parse(f);

        Universe universe;
        universe.load(universeJson);

        auto modelMatrix = glm::mat4(1.0f);

        double lastCursorX = 0.0, lastCursorY = 0.0;
        glfwGetCursorPos(window, &lastCursorX, &lastCursorY);

        constexpr auto kCircleMeshVertexCount = 20;
        Mesh circleMesh;
        {
            constexpr auto kRadius = 0.1;

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
            circleMesh.setVertexData(std::as_bytes(std::span{verts}));
            const std::array<Mesh::VertexAttribute, 1> attributes = {Mesh::VertexAttribute{2, Mesh::Type::Float, 0}};
            circleMesh.setVertexAttributes(attributes, sizeof(glm::vec2));
        }

        GlyphCache glyphCache("DejaVuSans.ttf", 20);
        TileBatcher tileBatcher;

        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        auto currentTime = JulianClock::now();

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            // TODO: proper arcball camera
            double cursorX = 0.0, cursorY = 0.0;
            glfwGetCursorPos(window, &cursorX, &cursorY);
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
            {
                constexpr auto RotationSpeed = 0.025;
                if (cursorX != lastCursorX)
                {
                    const auto dx = static_cast<float>(RotationSpeed * (cursorX - lastCursorX));
                    modelMatrix = glm::rotate(glm::mat4(1.0), dx, glm::vec3(0.0, 1.0, 0.0)) * modelMatrix;
                }
                if (cursorY != lastCursorY)
                {
                    const auto dy = static_cast<float>(RotationSpeed * (cursorY - lastCursorY));
                    modelMatrix = glm::rotate(glm::mat4(1.0), dy, glm::vec3(1.0, 0.0, 0.0)) * modelMatrix;
                }
            }
            lastCursorX = cursorX;
            lastCursorY = cursorY;

            int width, height;
            glfwGetWindowSize(window, &width, &height);

            glViewport(0, 0, width, height);
            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            const auto projectionMatrix =
                glm::perspective(glm::radians(45.0f), static_cast<float>(width) / height, 0.1f, 100.0f);
            const auto viewMatrix =
                glm::lookAt(glm::vec3(0.0f, 0.0f, 8.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            const auto mvp = projectionMatrix * viewMatrix * modelMatrix;

            orbitProgram.use();
            orbitProgram.setUniform(orbitProgram.uniformLocation("mvp"), mvp);

            const auto worlds = universe.worlds();

            for (const auto &world : worlds)
            {
                world.body().renderOrbit();
            }

            billboardProgram.use();
            billboardProgram.setUniform(billboardProgram.uniformLocation("projectionMatrix"), projectionMatrix);
            billboardProgram.setUniform(billboardProgram.uniformLocation("viewMatrix"), viewMatrix);

            // sun
            billboardProgram.setUniform(billboardProgram.uniformLocation("mvp"), mvp);
            billboardProgram.setUniform(billboardProgram.uniformLocation("modelMatrix"), modelMatrix);
            circleMesh.draw(Mesh::Primitive::LineLoop, 0, kCircleMeshVertexCount);

            tileBatcher.reset();
            for (const auto &world : worlds)
            {
                const auto &body = world.body();
                const auto position = body.position(currentTime);
                const auto bodyModelMatrix = modelMatrix * glm::translate(glm::mat4(1.0), position);
                const auto mvp = projectionMatrix * viewMatrix * bodyModelMatrix;
                billboardProgram.setUniform(billboardProgram.uniformLocation("mvp"), mvp);
                billboardProgram.setUniform(billboardProgram.uniformLocation("modelMatrix"), bodyModelMatrix);
                circleMesh.draw(Mesh::Primitive::LineLoop, 0, kCircleMeshVertexCount);

                {
                    auto positionProjected = mvp * glm::vec4(0.0, 0.0, 0.0, 1.0);
                    glm::vec2 p;
                    p.x = 0.5f * ((positionProjected.x / positionProjected.w) + 1.0) * width;
                    p.y = (1.0f - 0.5f * ((positionProjected.y / positionProjected.w) + 1.0)) * height;

                    p.x += 5.0;
                    p.y -= glyphCache.pixelHeight();

                    const auto name = world.name();
                    for (size_t index = 0; const char ch : world.name())
                    {
                        const auto glyph = glyphCache.getGlyph(std::toupper(ch));
                        if (glyph.has_value())
                        {
                            const auto topLeft = p + glyph->topLeft;
                            const auto bottomRight = topLeft + glm::vec2(glyph->width, glyph->height);
                            tileBatcher.setTexture(glyph->texture);
                            tileBatcher.addTile({topLeft, glyph->texCoords.topLeft},
                                                {bottomRight, glyph->texCoords.bottomRight});
                            p += glm::vec2(glyph->advance, 0);
                            if (index < name.size() - 1)
                            {
                                p += glm::vec2(glyphCache.kernAdvance(ch, name[index + 1]), 0);
                            }
                        }
                        ++index;
                    }
                }
            }

            {
                textProgram.use();
                const auto mvp = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f);
                textProgram.setUniform(textProgram.uniformLocation("mvp"), mvp);
                tileBatcher.blit();
            }

            glfwSwapBuffers(window);

            // currentTime += JulianClock::duration{0.5};
            currentTime += JulianClock::duration{0.25};
        }
    }

    glfwTerminate();
}
