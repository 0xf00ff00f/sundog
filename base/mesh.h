#pragma once

#include "glhelpers.h"

#include <span>

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
    void setVertexData(std::span<const std::byte> vertexData, std::size_t vertexCount);
    void setIndexData(std::span<const std::uint32_t> indexData);

    enum class Primitive : GLenum
    {
        Triangles = GL_TRIANGLES,
        TriangleStrip = GL_TRIANGLE_STRIP,
        TriangleFan = GL_TRIANGLE_FAN,
        Lines = GL_LINES,
        LineLoop = GL_LINE_LOOP,
        Points = GL_POINTS
    };
    void draw(Primitive primitive) const;

private:
    gl::Buffer m_vertexBuffer;
    gl::Buffer m_indexBuffer;
    gl::VertexArray m_vertexArray;
    std::size_t m_vertexCount{0};
    std::size_t m_indexCount{0};
};
