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
    m_vertexArray.unbind();
}

void Mesh::setVertexData(std::span<const std::byte> vertexData, std::size_t vertexCount)
{
    m_vertexBuffer.bind();
    m_vertexBuffer.data(vertexData);

    m_vertexCount = vertexCount;
}

void Mesh::setIndexData(std::span<const std::uint32_t> indexData)
{
    m_indexBuffer.bind();
    m_indexBuffer.data(std::as_bytes(indexData));

    m_indexCount = indexData.size();
}

void Mesh::draw(Primitive primitive) const
{
    m_vertexArray.bind();
    if (m_indexCount == 0)
        glDrawArrays(static_cast<GLenum>(primitive), 0, m_vertexCount);
    else
        glDrawElements(static_cast<GLenum>(primitive), m_indexCount, GL_UNSIGNED_INT, nullptr);
    m_vertexArray.unbind();
}
