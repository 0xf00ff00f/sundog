module;

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <print>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

export module glhelpers;

export namespace gl
{

class Buffer
{
public:
    enum class Target : GLenum
    {
        ArrayBuffer = GL_ARRAY_BUFFER
    };

    enum class Usage
    {
        StaticDraw = GL_STATIC_DRAW
    };

    explicit Buffer(Target target);
    ~Buffer();

    Buffer(Buffer &&other);
    Buffer &operator=(Buffer &&other);

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;

    GLuint handle() const { return m_handle; }

    void bind() const;
    void data(std::span<const std::byte> bytes, Usage usage);

private:
    Target m_target;
    GLuint m_handle = 0;
};

class VertexArray
{
public:
    VertexArray();
    ~VertexArray();

    VertexArray(VertexArray &&other);
    VertexArray &operator=(VertexArray &&other);

    VertexArray(const VertexArray &) = delete;
    VertexArray &operator=(const VertexArray &) = delete;

    GLuint handle() const { return m_handle; }

    void bind() const;

private:
    GLuint m_handle = 0;
};

class ShaderProgram
{
public:
    ShaderProgram();
    ~ShaderProgram();

    ShaderProgram(ShaderProgram &&other);
    ShaderProgram &operator=(ShaderProgram &&other);

    ShaderProgram(const ShaderProgram &) = delete;
    ShaderProgram &operator=(const ShaderProgram &) = delete;

    GLuint handle() const { return m_handle; }

    enum class ShaderType : GLenum
    {
        VertexShader = GL_VERTEX_SHADER,
        FragmentShader = GL_FRAGMENT_SHADER
    };

    bool attachShader(ShaderType type, std::string_view source) const;
    bool link() const;
    void use() const;

    int uniformLocation(std::string_view uniform) const;

    void setUniform(int location, float value) const;
    void setUniform(int location, const glm::vec2 &value) const;
    void setUniform(int location, const glm::vec3 &value) const;
    void setUniform(int location, const glm::vec4 &value) const;
    void setUniform(int location, const glm::mat3 &value) const;
    void setUniform(int location, const glm::mat4 &value) const;

private:
    GLuint m_handle = 0;
};

Buffer::Buffer(Target target)
    : m_target(target)
{
    glGenBuffers(1, &m_handle);
}

Buffer::~Buffer()
{
    if (m_handle)
        glDeleteBuffers(1, &m_handle);
}

Buffer::Buffer(Buffer &&other)
    : m_handle(std::exchange(other.m_handle, 0))
{
}

Buffer &Buffer::operator=(Buffer &&other)
{
    if (this != &other)
    {
        if (m_handle)
            glDeleteBuffers(1, &m_handle);
        m_handle = std::exchange(other.m_handle, 0);
    }
    return *this;
}

void Buffer::bind() const
{
    glBindBuffer(static_cast<GLenum>(m_target), m_handle);
}

void Buffer::data(std::span<const std::byte> bytes, Usage usage)
{
    glBufferData(static_cast<GLenum>(m_target), bytes.size(), bytes.data(), static_cast<GLenum>(usage));
}

VertexArray::VertexArray()
{
    glGenVertexArrays(1, &m_handle);
}

VertexArray::~VertexArray()
{
    if (m_handle)
        glDeleteVertexArrays(1, &m_handle);
}

VertexArray::VertexArray(VertexArray &&other)
    : m_handle(std::exchange(other.m_handle, 0))
{
}

VertexArray &VertexArray::operator=(VertexArray &&other)
{
    if (this != &other)
    {
        if (m_handle)
            glDeleteVertexArrays(1, &m_handle);
        m_handle = std::exchange(other.m_handle, 0);
    }
    return *this;
}

void VertexArray::bind() const
{
    glBindVertexArray(m_handle);
}

ShaderProgram::ShaderProgram()
    : m_handle(glCreateProgram())
{
}

ShaderProgram::~ShaderProgram()
{
    glDeleteProgram(m_handle);
}

ShaderProgram::ShaderProgram(ShaderProgram &&other)
    : m_handle(std::exchange(other.m_handle, 0))
{
}

ShaderProgram &ShaderProgram::operator=(ShaderProgram &&other)
{
    if (this != &other)
    {
        if (m_handle)
            glDeleteProgram(m_handle);
        m_handle = std::exchange(other.m_handle, 0);
    }
    return *this;
}

bool ShaderProgram::attachShader(ShaderType type, std::string_view source) const
{
    GLuint shader = glCreateShader(static_cast<GLenum>(type));

    std::array<std::string_view, 2> sources = {
        "#version 410 core\n",
        source,
    };
    // clang-format off
    const std::vector<const GLchar *> strings = sources
        | std::views::transform([](std::string_view s) { return reinterpret_cast<const GLchar *>(s.data()); })
        | std::ranges::to<std::vector>();
    const std::vector<GLint> lengths = sources
        | std::views::transform([](std::string_view s) { return static_cast<GLint>(s.size()); })
        | std::ranges::to<std::vector>();
    // clang-format on
    glShaderSource(shader, sources.size(), strings.data(), lengths.data());
    glCompileShader(shader);

    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        std::array<char, 1024> log;
        GLsizei length = 0;
        glGetShaderInfoLog(shader, log.size(), &length, log.data());
        std::println(stderr, "Failed to compile shader: {}", std::string_view(log.data(), length));
        return false;
    }

    glAttachShader(m_handle, shader);

    return true;
}

bool ShaderProgram::link() const
{
    glLinkProgram(m_handle);

    GLint status = GL_FALSE;
    glGetProgramiv(m_handle, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        std::array<char, 1024> log;
        GLsizei length;
        glGetProgramInfoLog(m_handle, log.size(), &length, log.data());
        std::println(stderr, "Failed to link program: {}", std::string_view(log.data(), length));
        return false;
    }

    return true;
}

void ShaderProgram::use() const
{
    glUseProgram(m_handle);
}

int ShaderProgram::uniformLocation(std::string_view uniform) const
{
    return glGetUniformLocation(m_handle, std::string(uniform).c_str());
}

void ShaderProgram::setUniform(int location, float value) const
{
    glUniform1f(location, value);
}

void ShaderProgram::setUniform(int location, const glm::vec2 &value) const
{
    glUniform2fv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::setUniform(int location, const glm::vec3 &value) const
{
    glUniform3fv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::setUniform(int location, const glm::vec4 &value) const
{
    glUniform4fv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::setUniform(int location, const glm::mat3 &value) const
{
    glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderProgram::setUniform(int location, const glm::mat4 &value) const
{
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

} // namespace gl
