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

// https://farside.ph.utexas.edu/teaching/celestial/Celestial/node34.html
// http://astro.if.ufrgs.br/trigesf/position.html

struct OrbitalElements
{
    float semiMajorAxis = 1.0f;
    float eccentricity = 0.0f;
    float inclination = 0.0f;
    float longitudePerihelion = 0.0f;
    float longitudeAscendingNode = 0.0f;
};

// longitudePerihelion = longitudeAscendingNode + argumentPerihelion

struct Body
{
public:
    Body();

    void setOrbitalElements(const OrbitalElements &orbit);
    void renderOrbit() const;

private:
    void initializeOrbitMesh();

    static constexpr auto kOrbitVertexCount = 300;

    OrbitalElements m_orbit;
    Mesh m_orbitMesh;
};

Body::Body()
{
    initializeOrbitMesh();
}

void Body::setOrbitalElements(const OrbitalElements &orbit)
{
    m_orbit = orbit;
    initializeOrbitMesh();
}

void Body::initializeOrbitMesh()
{
    const auto semiMajorAxis = m_orbit.semiMajorAxis;
    const auto semiMinorAxis = semiMajorAxis * std::sqrt(1.0 - m_orbit.eccentricity * m_orbit.eccentricity);
    const auto focus = std::sqrt(semiMajorAxis * semiMajorAxis - semiMinorAxis * semiMinorAxis);

    const auto argumentPerihelion = m_orbit.longitudePerihelion - m_orbit.longitudeAscendingNode;

    const auto r0 = glm::rotate(glm::mat4(1.0), glm::radians(argumentPerihelion), glm::vec3(0.0, 0.0, 1.0));
    const auto r1 = glm::rotate(glm::mat4(1.0), glm::radians(m_orbit.inclination), glm::vec3(1.0, 0.0, 0.0));
    const auto r2 = glm::rotate(glm::mat4(1.0), glm::radians(m_orbit.longitudeAscendingNode), glm::vec3(0.0, 0.0, 1.0));
    const auto rotationMatrix = r0 * r1 * r2;

    struct Vertex
    {
        glm::vec3 position;
    };
    // clang-format off
    const std::vector<Vertex> verts =
        std::views::iota(0, kOrbitVertexCount)
        | std::views::transform([semiMajorAxis, semiMinorAxis, focus, &rotationMatrix](const std::size_t i) -> Vertex {
              const auto t = i * 2.0f * glm::pi<float>() / kOrbitVertexCount;
              const auto x = semiMajorAxis * glm::cos(t) + focus;
              const auto y = semiMinorAxis * glm::sin(t);
              const auto position = glm::vec3(rotationMatrix * glm::vec4(x, y, 0.0f, 1.0f));
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
            {0.3871, 0.20564, 7.006, 77.46, 48.34},  // Mercury
            {0.7233, 0.00676, 3.398, 131.77, 76.67}, // Venus
            {1.0000, 0.01673, 0.000, 102.93, 0.000}, // Earth
            {1.5237, 0.09337, 1.852, 336.08, 49.71}, // Mars
            {5.2025, 0.04854, 1.299, 14.27, 100.29}, // Jupiter
            {9.5415, 0.05551, 2.494, 92.86, 113.64}, // Saturn
            {19.188, 0.04686, 0.773, 172.43, 73.96}, // Uranus
            {30.070, 0.00895, 1.770, 46.68, 131.79}, // Neptune
            {17.93, 0.9679, 162.19, 171.37, 59.11},  // Halley
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
                glm::lookAt(glm::vec3(0.0f, 0.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            const auto mvp = projectionMatrix * viewMatrix * modelMatrix;

            program.use();
            program.setUniform(program.uniformLocation("mvp"), mvp);

            for (const auto &body : bodies)
                body.renderOrbit();

            glfwSwapBuffers(window);
        }
    }

    glfwTerminate();
}
