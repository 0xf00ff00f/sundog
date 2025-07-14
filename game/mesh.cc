module;

#include <glad/gl.h>

#include <span>
#include <ranges>

export module mesh;

import glhelpers;

export class Mesh
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

    enum class Primitive : GLenum
    {
        Triangles = GL_TRIANGLES,
        Lines = GL_LINES,
        LineLoop = GL_LINE_LOOP
    };
    void draw(Primitive primitive, std::size_t firstVertex, std::size_t vertexCount) const;

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

void Mesh::draw(Primitive primitive, std::size_t firstVertex, std::size_t vertexCount) const
{
    m_vertexArray.bind();
    glDrawArrays(static_cast<GLenum>(primitive), firstVertex, vertexCount);
}
