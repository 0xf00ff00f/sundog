#include <glad/gl.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/string_cast.hpp>

#include <cassert>
#include <stdio.h>

import glhelpers;
import mesh;

import std;

namespace
{
constexpr auto kEarthYearInDays = 365.2425;
}

// https://farside.ph.utexas.edu/teaching/celestial/Celestial/node34.html
// http://astro.if.ufrgs.br/trigesf/position.html
// http://www.davidcolarusso.com/astro/

struct OrbitalElements
{
    float semiMajorAxis = 1.0f;
    float eccentricity = 0.0f;
    float inclination = 0.0f;            // degrees
    float longitudePerihelion = 0.0f;    // degrees
    float longitudeAscendingNode = 0.0f; // degrees
    float meanAnomalyAtEpoch = 0.0f;     // degrees
};
// longitudePerihelion = longitudeAscendingNode + argumentPerihelion

using Days = std::chrono::duration<float, std::ratio<60 * 60 * 24>>;

Days daysSinceEpoch(std::chrono::time_point<std::chrono::system_clock> t)
{
    using namespace std::chrono_literals;
    using namespace std::chrono;
    const auto startEpoch = 2000y / January / 0;
    return duration_cast<Days>(sys_days{std::chrono::floor<days>(t)} - sys_days{startEpoch});
}

struct Body
{
public:
    Body();

    void setOrbitalElements(const OrbitalElements &orbit);
    void renderOrbit() const;
    float meanAnomaly(Days day) const;      // radians
    float eccentricAnomaly(Days day) const; // radians
    glm::vec3 position(Days day) const;

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
    m_orbit = orbit;
    m_period = std::pow(m_orbit.semiMajorAxis, 3.0 / 2.0) * kEarthYearInDays;
    initializeOrbitMesh();
}

float Body::meanAnomaly(Days day) const
{
    const float Mepoch = glm::radians(m_orbit.meanAnomalyAtEpoch);
    return Mepoch + 2.0 * glm::pi<float>() * day.count() / m_period;
}

float Body::eccentricAnomaly(Days day) const
{
    const float e = m_orbit.eccentricity;
    const float M = meanAnomaly(day);

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

glm::vec3 Body::position(Days day) const
{
    const auto E = eccentricAnomaly(day);

    const auto a = m_orbit.semiMajorAxis;
    const auto e = m_orbit.eccentricity;

    // position in orbit
    const auto x = a * (std::cos(E) - e);
    const auto y = a * (std::sqrt(1.0 - e * e) * std::sin(E));

    const auto position = orbitRotationMatrix() * glm::vec3(x, y, 0.0);
    std::println("position={}", glm::to_string(position));

#if 1
    {
        const auto w = glm::radians(m_orbit.longitudePerihelion - m_orbit.longitudeAscendingNode);
        const auto i = glm::radians(m_orbit.inclination);
        const auto N = glm::radians(m_orbit.longitudeAscendingNode);

        const auto v = std::atan2(y, x);
        const auto r = std::sqrt(x * x + y * y);
        const auto xh = r * (std::cos(N) * std::cos(v + w) - std::sin(N) * std::sin(v + w) * std::cos(i));
        const auto yh = r * (std::sin(N) * std::cos(v + w) + std::cos(N) * std::sin(v + w) * std::cos(i));
        const auto zh = r * std::sin(v + w) * std::sin(i);

        std::println("expected={}", glm::to_string(glm::vec3(xh, yh, zh)));
    }
#endif

    return position;
}

glm::mat3 Body::orbitRotationMatrix() const
{
    const auto w = glm::radians(m_orbit.longitudePerihelion - m_orbit.longitudeAscendingNode);
    const auto rw = glm::mat3(glm::rotate(glm::mat4(1.0), w, glm::vec3(0.0, 0.0, 1.0)));

    const auto i = glm::radians(m_orbit.inclination);
    const auto ri = glm::mat3(glm::rotate(glm::mat4(1.0), i, glm::vec3(1.0, 0.0, 0.0)));

    const auto N = glm::radians(m_orbit.longitudeAscendingNode);
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

        const char *vertexShader = R"(
layout(location=0) in vec3 position;

uniform mat4 mvp;

out vec3 vs_color;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
}
)";
        const char *fragmentShader = R"(
out vec4 fragColor;

void main() {
    fragColor = vec4(1.0);
}
)";
        ShaderProgram program;
        if (!program.attachShader(ShaderProgram::ShaderType::FragmentShader, fragmentShader))
        {
            glfwTerminate();
            return 1;
        }
        if (!program.attachShader(ShaderProgram::ShaderType::VertexShader, vertexShader))
        {
            glfwTerminate();
            return 1;
        }
        if (!program.link())
        {
            glfwTerminate();
            return 1;
        }

        static const std::vector<OrbitalElements> orbits = {
            // semiMajorAxis, eccentricity, inclination, longitudePerihelion, longitudeAscendingNode, meanAnomalyAtEpoch
            // a, e, i, w + N, N
            {0.3871, 0.20564, 7.006, 77.46, 48.34, 168.6562},  // Mercury
            {0.7233, 0.00676, 3.398, 131.77, 76.67, 48.0052},  // Venus
            {1.0000, 0.01673, 0.000, 102.93, 0.000, 0.00},     // Earth
            {1.5237, 0.09337, 1.852, 336.08, 49.71, 18.6021},  // Mars
            {5.2025, 0.04854, 1.299, 14.27, 100.29, 19.8950},  // Jupiter
            {9.5415, 0.05551, 2.494, 92.86, 113.64, 316.9670}, // Saturn
            {19.188, 0.04686, 0.773, 172.43, 73.96, 142.5905}, // Uranus
            {30.070, 0.00895, 1.770, 46.68, 131.79, 260.2471}, // Neptune
        };

        // clang-format off
        const std::vector<Body> bodies = orbits
                                         | std::views::transform([](const OrbitalElements &orbit) {
                                               Body body;
                                               body.setOrbitalElements(orbit);
                                               return body;
                                           })
                                         | std::ranges::to<std::vector>();
        // clang-format on

        auto modelMatrix = glm::mat4(1.0f);

        double lastCursorX = 0.0, lastCursorY = 0.0;
        glfwGetCursorPos(window, &lastCursorX, &lastCursorY);

        {
            using namespace std::chrono_literals;
            std::println("{}", std::chrono::duration_cast<Days>(2s).count());
        }

        Mesh bodyMesh;
        {
            constexpr auto kSize = 0.2;
            static const std::vector<glm::vec3> vertices = {
                {-kSize, 0.0, 0.0}, {kSize, 0.0, 0.0},  {0.0, -kSize, 0.0},
                {0.0, kSize, 0.0},  {0.0, 0.0, -kSize}, {0.0, 0.0, kSize},
            };
            bodyMesh.setVertexData(std::as_bytes(std::span{vertices}));
            const std::array<Mesh::VertexAttribute, 1> attributes = {Mesh::VertexAttribute{3, Mesh::Type::Float, 0}};
            bodyMesh.setVertexAttributes(attributes, sizeof(glm::vec3));
        }

        auto currentTime = daysSinceEpoch(std::chrono::system_clock::now());

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
                glm::lookAt(glm::vec3(0.0f, 0.0f, 16.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            const auto mvp = projectionMatrix * viewMatrix * modelMatrix;

            program.use();
            program.setUniform(program.uniformLocation("mvp"), mvp);

            for (const auto &body : bodies)
            {
                body.renderOrbit();
            }

            for (const auto &body : bodies)
            {
                const auto position = body.position(currentTime);
                const auto bodyModelMatrix = modelMatrix * glm::translate(glm::mat4(1.0), position);
                const auto mvp = projectionMatrix * viewMatrix * bodyModelMatrix;
                program.setUniform(program.uniformLocation("mvp"), mvp);
                bodyMesh.draw(Mesh::Primitive::Lines, 0, 6);
            }

            glfwSwapBuffers(window);

            currentTime += Days{0.5};
        }
    }

    glfwTerminate();
}
