#include <glad/gl.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <cassert>
#include <stdio.h>

import glhelpers;
import mesh;

import std;

struct Orbit
{
    float semiMajorAxis = 1.0;
    float eccentricity = 0.0;
};

struct Body
{
public:
    Body();

    void setOrbit(const Orbit &orbit);
    void renderOrbit() const;

private:
    void initializeOrbitMesh();

    static constexpr auto kOrbitVertexCount = 30;

    Orbit m_orbit;
    Mesh m_orbitMesh;
};

Body::Body()
{
    initializeOrbitMesh();
}

void Body::setOrbit(const Orbit &orbit)
{
    m_orbit = orbit;
    initializeOrbitMesh();
}

void Body::initializeOrbitMesh()
{
    const auto semiMajorAxis = m_orbit.semiMajorAxis;
    const auto semiMinorAxis = semiMajorAxis * std::sqrt(1.0 - m_orbit.eccentricity);

    struct Vertex
    {
        glm::vec2 position;
    };
    // clang-format off
    const std::vector<Vertex> verts =
        std::views::iota(0, kOrbitVertexCount)
        | std::views::transform([semiMajorAxis, semiMinorAxis](const std::size_t i) -> Vertex {
              const auto t = i * 2.0f * glm::pi<float>() / kOrbitVertexCount;
              const auto x = semiMajorAxis * glm::cos(t);
              const auto y = semiMinorAxis * glm::sin(t);
              return Vertex{.position = glm::vec2{x, y}};
          })
        | std::ranges::to<std::vector>();
    // clang-format on
    m_orbitMesh.setVertexData(std::as_bytes(std::span{verts}));

    const std::array<Mesh::VertexAttribute, 1> attributes = {
        Mesh::VertexAttribute{2, Mesh::Type::Float, offsetof(Vertex, position)},
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

    GLFWwindow *window = glfwCreateWindow(400, 400, "Hello", nullptr, nullptr);
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

        struct Vertex
        {
            glm::vec3 position;
            glm::vec3 color;
        };
        const std::array<Vertex, 3> vertices = {
            Vertex{.position = glm::vec3{0.0f, 0.5f, 0.0f}, .color = glm::vec3{1.0, 0.0, 0.0}},
            Vertex{.position = glm::vec3{0.5f, -0.5f, 0.0f}, .color = glm::vec3{0.0, 1.0, 0.0}},
            Vertex{.position = glm::vec3{-0.5f, -0.5f, 0.0f}, .color = glm::vec3{0.0, 0.0, 1.0}}};

        const std::array<Mesh::VertexAttribute, 2> attributes = {
            Mesh::VertexAttribute{3, Mesh::Type::Float, offsetof(Vertex, position)},
            Mesh::VertexAttribute{3, Mesh::Type::Float, offsetof(Vertex, color)},
        };

        Mesh mesh;
        mesh.setVertexData(std::as_bytes(std::span{vertices}));
        mesh.setVertexAttributes(attributes, sizeof(Vertex));

        const char *vertexShader = R"(
layout(location=0) in vec2 position;

out vec3 vs_color;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
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

        Body body;
        body.setOrbit(Orbit{.semiMajorAxis = 0.75f, .eccentricity = 0.75f});

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            program.use();
            body.renderOrbit();

            glfwSwapBuffers(window);
        }
    }

    glfwTerminate();
}
