#include "tile_batcher.h"

#include <algorithm>
#include <ranges>

namespace
{
constexpr auto kTilesPerBuffer = 512 * 1024;

struct GLVertex
{
    glm::vec2 position;
    glm::vec2 texCoords;
};
} // namespace

TileBatcher::TileBatcher()
    : m_vertexBuffer(gl::Buffer::Target::ArrayBuffer, gl::Buffer::Usage::DynamicDraw)
    , m_indexBuffer(gl::Buffer::Target::ElementArrayBuffer, gl::Buffer::Usage::StaticDraw)
{
    std::vector<uint32_t> indices(kTilesPerBuffer * 6);
    for (size_t i = 0; i < kTilesPerBuffer; ++i)
    {
        indices[i * 6 + 0] = i * 4 + 0;
        indices[i * 6 + 1] = i * 4 + 1;
        indices[i * 6 + 2] = i * 4 + 2;

        indices[i * 6 + 3] = i * 4 + 2;
        indices[i * 6 + 4] = i * 4 + 3;
        indices[i * 6 + 5] = i * 4 + 0;
    }
    m_indexBuffer.bind();
    m_indexBuffer.data(std::as_bytes(std::span(indices)));

    m_vertexArray.bind();
    m_vertexBuffer.bind();
    m_indexBuffer.bind();

    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex),
                          reinterpret_cast<GLvoid *>(offsetof(GLVertex, position)));

    // texCoord
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex),
                          reinterpret_cast<GLvoid *>(offsetof(GLVertex, texCoords)));

    m_vertexArray.unbind();
}

TileBatcher::~TileBatcher() = default;

void TileBatcher::reset()
{
    m_tiles.clear();
    m_curTexture = nullptr;
}

void TileBatcher::setTexture(const gl::AbstractTexture *texture)
{
    m_curTexture = texture;
}

void TileBatcher::addTile(const Vertex &topLeft, const Vertex &bottomRight, int depth)
{
    m_tiles.emplace_back(topLeft, bottomRight, m_curTexture, depth);
}

void TileBatcher::blit() const
{
    std::vector<const Tile *> sortedTiles =
        m_tiles | std::views::transform([](const Tile &tile) { return &tile; }) | std::ranges::to<std::vector>();
    std::ranges::sort(sortedTiles, [](const Tile *lhs, const Tile *rhs) {
        return std::tie(lhs->depth, lhs->texture) < std::tie(rhs->depth, rhs->texture);
    });

    m_vertexBuffer.bind();
    m_vertexArray.bind();

    auto batchStart = sortedTiles.begin();
    while (batchStart != sortedTiles.end())
    {
        const auto *batchTexture = (*batchStart)->texture;
        const auto batchEnd = std::find_if(batchStart + 1, sortedTiles.end(),
                                           [batchTexture](const Tile *tile) { return tile->texture != batchTexture; });
        while (batchStart != batchEnd)
        {
            if (!m_bufferAllocated || m_tileIndex == kTilesPerBuffer)
            {
                m_vertexBuffer.allocate(kTilesPerBuffer * 4 * sizeof(GLVertex));
                m_tileIndex = 0;
                m_bufferAllocated = true;
            }
            const auto tileCount =
                std::min<std::size_t>(std::distance(batchStart, batchEnd), kTilesPerBuffer - m_tileIndex);

            auto *vertexData = m_vertexBuffer.mapRange<GLVertex>(
                m_tileIndex * 4, tileCount * 4, gl::Buffer::Access::Write | gl::Buffer::Access::Unsynchronized);
            for (auto it = batchStart; it != batchStart + tileCount; ++it)
            {
                const auto *tile = *it;
                const auto &topLeft = tile->topLeft;
                const auto &bottomRight = tile->bottomRight;

                vertexData->position = {topLeft.position.x, topLeft.position.y};
                vertexData->texCoords = {topLeft.texCoords.x, topLeft.texCoords.y};
                ++vertexData;

                vertexData->position = {bottomRight.position.x, topLeft.position.y};
                vertexData->texCoords = {bottomRight.texCoords.x, topLeft.texCoords.y};
                ++vertexData;

                vertexData->position = {bottomRight.position.x, bottomRight.position.y};
                vertexData->texCoords = {bottomRight.texCoords.x, bottomRight.texCoords.y};
                ++vertexData;

                vertexData->position = {topLeft.position.x, bottomRight.position.y};
                vertexData->texCoords = {topLeft.texCoords.x, bottomRight.texCoords.y};
                ++vertexData;
            }
            m_vertexBuffer.unmap();

            batchTexture->bind();
            glDrawElements(GL_TRIANGLES, 6 * tileCount, GL_UNSIGNED_INT,
                           reinterpret_cast<void *>(m_tileIndex * 6 * sizeof(uint32_t)));

            m_tileIndex += tileCount;
            batchStart += tileCount;
        }
    }
}
