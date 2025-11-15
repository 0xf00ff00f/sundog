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
