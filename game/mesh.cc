#include "mesh.h"

#include <ranges>

Mesh::Mesh()
    : m_vertexBuffer(gl::Buffer::Target::ArrayBuffer, gl::Buffer::Usage::StaticDraw)
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
    m_vertexBuffer.data(vertexData);
}

void Mesh::draw(Primitive primitive, std::size_t firstVertex, std::size_t vertexCount) const
{
    m_vertexArray.bind();
    glDrawArrays(static_cast<GLenum>(primitive), firstVertex, vertexCount);
}
