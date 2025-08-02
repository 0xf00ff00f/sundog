#pragma once

#include "image.h"

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
        ArrayBuffer = GL_ARRAY_BUFFER,
        ElementArrayBuffer = GL_ELEMENT_ARRAY_BUFFER
    };

    enum class Usage : GLenum
    {
        StaticDraw = GL_STATIC_DRAW,
        DynamicDraw = GL_DYNAMIC_DRAW,
        StreamDraw = GL_STREAM_DRAW
    };

    enum class Access : unsigned
    {
        Read = GL_MAP_READ_BIT,
        Write = GL_MAP_WRITE_BIT,
        Unsynchronized = GL_MAP_UNSYNCHRONIZED_BIT
    };

    explicit Buffer(Target target, Usage usage);
    ~Buffer();

    Buffer(Buffer &&other);
    Buffer &operator=(Buffer &&other);

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;

    GLuint handle() const { return m_handle; }

    void bind() const;
    void unbind() const;
    void data(std::span<const std::byte> bytes) const;
    void allocate(size_t size) const;

    template<typename T>
    T *mapRange(std::size_t offset, std::size_t length, Access access) const
    {
        bind();
        return static_cast<T *>(glMapBufferRange(static_cast<GLenum>(m_target), offset * sizeof(T), length * sizeof(T),
                                                 static_cast<GLbitfield>(access)));
    }
    void unmap() const;

private:
    void data(size_t size, const std::byte *data) const;

    Target m_target;
    Usage m_usage;
    GLuint m_handle = 0;
};

constexpr Buffer::Access operator&(Buffer::Access x, Buffer::Access y)
{
    return static_cast<Buffer::Access>(static_cast<unsigned>(x) & static_cast<unsigned>(y));
}

constexpr Buffer::Access &operator&=(Buffer::Access &x, Buffer::Access y)
{
    return x = x & y;
}

constexpr Buffer::Access operator|(Buffer::Access x, Buffer::Access y)
{
    return static_cast<Buffer::Access>(static_cast<unsigned>(x) | static_cast<unsigned>(y));
}

constexpr Buffer::Access &operator|=(Buffer::Access &x, Buffer::Access y)
{
    return x = x | y;
}

class AbstractTexture
{
public:
    virtual ~AbstractTexture() = default;

    virtual void bind() const = 0;
};

class Texture : public AbstractTexture
{
public:
    explicit Texture(const Image<uint32_t> &image);
    explicit Texture(size_t width, size_t height);
    ~Texture();

    Texture(Texture &&other);
    Texture &operator=(Texture &&other);

    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;

    size_t width() const { return m_width; }
    size_t height() const { return m_height; }

    GLuint handle() const { return m_handle; }

    void bind() const override;

    enum class Filter : GLint
    {
        Nearest = GL_NEAREST,
        Linear = GL_LINEAR
    };
    void setMinificationFilter(Filter filter) const;
    void setMagnificationFilter(Filter filter) const;

    enum class WrapMode : GLint
    {
        Repeat = GL_REPEAT,
        MirroredRepeat = GL_MIRRORED_REPEAT,
        ClampToEdge = GL_CLAMP_TO_EDGE
    };
    void setWrapModeS(WrapMode wrap) const;
    void setWrapModeT(WrapMode wrap) const;

    void data(std::span<const std::byte> bytes) const;

private:
    GLuint m_handle = 0;
    size_t m_width = 0;
    size_t m_height = 0;
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
    static void unbind();

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
