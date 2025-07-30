#pragma once

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string_view>
#include <span>

namespace gl
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

} // namespace gl
