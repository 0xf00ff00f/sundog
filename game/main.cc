#include <glad/gl.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <cassert>
#include <stdio.h>

import glhelpers;
import std;

class Mesh
{
public:
    Mesh();
    ~Mesh();

    Mesh(Mesh &&other);
    Mesh &operator=(Mesh &&other);

    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;

    enum class Type : GLenum
    {
        Float = GL_FLOAT
    };
    struct VertexAttribute
    {
        std::size_t size;
        Type type;
        std::size_t offset;
    };
    void setVertexAttributes(std::span<const VertexAttribute> attributes, std::size_t stride);
    void setVertexData(std::span<const std::byte> vertexData);
    void draw(std::size_t firstVertex, std::size_t vertexCount);

private:
    gl::Buffer m_vertexBuffer;
    gl::VertexArray m_vertexArray;
};

Mesh::Mesh()
    : m_vertexBuffer(gl::Buffer::Target::ArrayBuffer)
{
}

Mesh::~Mesh() = default;

Mesh::Mesh(Mesh &&other) = default;

Mesh &Mesh::operator=(Mesh &&other) = default;

void Mesh::setVertexAttributes(std::span<const VertexAttribute> attributes, std::size_t stride)
{
    m_vertexArray.bind();
    m_vertexBuffer.bind();
    // where's std::views::enumerate?
    for (const auto &&[index, attribute] : std::views::zip(std::views::iota(0), attributes))
    {
        glEnableVertexAttribArray(index);
        glVertexAttribPointer(index, attribute.size, static_cast<GLenum>(attribute.type), GL_FALSE, stride,
                              reinterpret_cast<const void *>(attribute.offset));
    }
}

void Mesh::setVertexData(std::span<const std::byte> vertexData)
{
    m_vertexBuffer.bind();
    m_vertexBuffer.data(vertexData, gl::Buffer::Usage::StaticDraw);
}

void Mesh::draw(std::size_t firstVertex, std::size_t vertexCount)
{
    m_vertexArray.bind();
    glDrawArrays(GL_TRIANGLES, firstVertex, vertexCount);
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
layout(location=0) in vec3 position;
layout(location=1) in vec3 color;

out vec3 vs_color;

void main() {
    vs_color = color;
    gl_Position = vec4(position, 1.0);
}
)";
        const char *fragmentShader = R"(
in vec3 vs_color;

out vec4 fragColor;

void main() {
    fragColor = vec4(vs_color, 1.0);
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

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            glClearColor(1.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            program.use();
            mesh.draw(0, 3);

            glfwSwapBuffers(window);
        }
    }

    glfwTerminate();
}
