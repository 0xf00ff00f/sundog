#pragma once

#include "glhelpers.h"

class TileBatcher
{
public:
    TileBatcher();
    ~TileBatcher();

    TileBatcher(TileBatcher &&) = delete;
    TileBatcher &operator=(TileBatcher &&) = delete;

    TileBatcher(const TileBatcher &) = delete;
    TileBatcher &operator=(const TileBatcher &) = delete;

    void reset();

    void setTexture(const gl::AbstractTexture *texture);
    struct Vertex
    {
        glm::vec2 position;
        glm::vec2 texCoords;
    };
    void addTile(const Vertex &topLeft, const Vertex &bottomRight, int depth = 0);

    void blit() const;

private:
    struct Tile
    {
        Vertex topLeft;
        Vertex bottomRight;
        const gl::AbstractTexture *texture;
        int depth;
    };
    const gl::AbstractTexture *m_curTexture = nullptr;
    std::vector<Tile> m_tiles;
    gl::Buffer m_vertexBuffer;
    gl::Buffer m_indexBuffer;
    gl::VertexArray m_vertexArray;
    gl::ShaderProgram m_shaderProgram;
    mutable bool m_bufferAllocated = false;
    mutable size_t m_tileIndex = 0; // index in buffer
};
