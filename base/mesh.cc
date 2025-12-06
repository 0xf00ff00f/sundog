#include "mesh.h"

#include <ranges>

Mesh::Mesh()
    : m_vertexBuffer(gl::Buffer::Target::ArrayBuffer, gl::Buffer::Usage::StaticDraw)
    , m_indexBuffer(gl::Buffer::Target::ElementArrayBuffer, gl::Buffer::Usage::StaticDraw)
{
}

Mesh::~Mesh() = default;

Mesh::Mesh(Mesh &&other) = default;

Mesh &Mesh::operator=(Mesh &&other) = default;

void Mesh::setVertexAttributes(std::span<const VertexAttribute> attributes, std::size_t stride)
{
    m_vertexArray.bind();
    m_vertexBuffer.bind();
    m_indexBuffer.bind();
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

void Mesh::setIndexData(std::span<const std::uint32_t> indexData)
{
    m_indexBuffer.bind();
    m_indexBuffer.data(std::as_bytes(indexData));
}

void Mesh::draw(Primitive primitive, std::size_t firstVertex, std::size_t vertexCount) const
{
    m_vertexArray.bind();
    glDrawArrays(static_cast<GLenum>(primitive), firstVertex, vertexCount);
}

void Mesh::drawElements(Primitive primitive, std::size_t count) const
{
    m_vertexArray.bind();
    glDrawElements(static_cast<GLenum>(primitive), count, GL_UNSIGNED_INT, nullptr);
}
