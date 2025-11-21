#include "painter.h"

#include "shader_manager.h"
#include "glyph_cache.h"
#include "sprite_texture_book.h"

#include <span>
#include <tuple>
#include <algorithm>
#include <print>

namespace
{
constexpr auto kSpriteSheetHeight = 1024;
constexpr auto kSpriteSheetWidth = 1024;
} // namespace

enum class VertexType
{
    PosColor,
    PosTexColor
};

template<typename VertexT>
class VertexIndexBuffer
{
public:
    VertexIndexBuffer()
        : m_vertexBuffer(gl::Buffer::Target::ArrayBuffer, gl::Buffer::Usage::DynamicDraw)
        , m_indexBuffer(gl::Buffer::Target::ElementArrayBuffer, gl::Buffer::Usage::StaticDraw)
    {
        m_vertexArray.bind();
        bindBuffers();

        size_t attribCount = 0;
        if constexpr (requires { VertexT{}.position; })
        {
            glEnableVertexAttribArray(attribCount);
            glVertexAttribPointer(attribCount, 2, GL_FLOAT, GL_FALSE, sizeof(VertexT),
                                  reinterpret_cast<GLvoid *>(offsetof(VertexT, position)));
            ++attribCount;
        }

        if constexpr (requires { VertexT{}.texCoords; })
        {
            glEnableVertexAttribArray(attribCount);
            glVertexAttribPointer(attribCount, 2, GL_FLOAT, GL_FALSE, sizeof(VertexT),
                                  reinterpret_cast<GLvoid *>(offsetof(VertexT, texCoords)));
            ++attribCount;
        }

        if constexpr (requires { VertexT{}.color; })
        {
            glEnableVertexAttribArray(attribCount);
            glVertexAttribPointer(attribCount, 4, GL_FLOAT, GL_FALSE, sizeof(VertexT),
                                  reinterpret_cast<GLvoid *>(offsetof(VertexT, color)));
            ++attribCount;
        }
    }

    void bindBuffers() const
    {
        m_vertexBuffer.bind();
        m_indexBuffer.bind();
    }

    void uploadData()
    {
        bindBuffers();
        m_vertexBuffer.data(std::as_bytes(std::span{vertices}));
        m_indexBuffer.data(std::as_bytes(std::span{indices}));
    }

    void draw() const
    {
        m_vertexArray.bind();
        bindBuffers();
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    }

    std::vector<VertexT> vertices;
    std::vector<uint32_t> indices;

private:
    gl::VertexArray m_vertexArray;
    gl::Buffer m_vertexBuffer;
    gl::Buffer m_indexBuffer;
};

struct VertexPosColor
{
    glm::vec2 position;
    glm::vec4 color;
};

struct VertexPosTexColor
{
    glm::vec2 position;
    glm::vec2 texCoords;
    glm::vec4 color;
};

using VertexPosColorBuffer = VertexIndexBuffer<VertexPosColor>;
using VertexPosTexColorBuffer = VertexIndexBuffer<VertexPosTexColor>;

class DrawCommand
{
public:
    explicit DrawCommand(int depth);
    virtual ~DrawCommand() = default;

    int depth() const { return m_depth; }

    virtual VertexType vertexType() const = 0;
    virtual const gl::AbstractTexture *texture() const = 0;

    virtual void dumpVertices(VertexPosColorBuffer &) const {};
    virtual void dumpVertices(VertexPosTexColorBuffer &) const {};

private:
    int m_depth;
};

class DrawCommandPosColor : public DrawCommand
{
public:
    using DrawCommand::DrawCommand;
};

class DrawCommandPosTexColor : public DrawCommand
{
public:
    using DrawCommand::DrawCommand;
};

class DrawPolyline : public DrawCommandPosColor
{
public:
    explicit DrawPolyline(std::span<const glm::vec2> verts, const glm::vec4 &color, float thickness, bool closed,
                          int depth);
    ~DrawPolyline() override = default;

    VertexType vertexType() const override { return VertexType::PosColor; }
    const gl::AbstractTexture *texture() const override { return nullptr; }

private:
    std::vector<glm::vec2> m_verts;
    glm::vec4 m_color;
    float m_thickness;
    bool m_closed;
};

class DrawFilledConvexPolygon : public DrawCommandPosColor
{
public:
    explicit DrawFilledConvexPolygon(std::span<const glm::vec2> verts, const glm::vec4 &color, int depth);
    ~DrawFilledConvexPolygon() override = default;

    VertexType vertexType() const override { return VertexType::PosColor; }
    const gl::AbstractTexture *texture() const override { return nullptr; }

    void dumpVertices(VertexPosColorBuffer &buffer) const override;

private:
    std::vector<glm::vec2> m_verts;
    glm::vec4 m_color;
};

class DrawSpriteBatch : public DrawCommandPosTexColor
{
public:
    explicit DrawSpriteBatch(const gl::AbstractTexture *texture, const glm::vec4 &color, int depth);

    struct Vertex
    {
        glm::vec2 position;
        glm::vec2 texCoords;
    };
    void addSprite(const Vertex &topLeft, const Vertex &bottomRight);

    VertexType vertexType() const override { return VertexType::PosTexColor; }
    const gl::AbstractTexture *texture() const override { return m_texture; }

    void dumpVertices(VertexPosTexColorBuffer &buffer) const override;

private:
    struct Quad
    {
        Vertex topLeft;
        Vertex bottomRight;
    };

    const gl::AbstractTexture *m_texture;
    glm::vec4 m_color;
    std::vector<Quad> m_quads;
};

DrawCommand::DrawCommand(int depth)
    : m_depth(depth)
{
}

DrawFilledConvexPolygon::DrawFilledConvexPolygon(std::span<const glm::vec2> verts, const glm::vec4 &color, int depth)
    : DrawCommandPosColor(depth)
    , m_verts(verts.begin(), verts.end())
    , m_color(color)
{
}

void DrawFilledConvexPolygon::dumpVertices(VertexPosColorBuffer &buffer) const
{
    if (m_verts.size() < 3)
        return;

    auto &vertices = buffer.vertices;
    auto &indices = buffer.indices;

    auto vertexIndex = buffer.vertices.size();

    for (const auto &pos : m_verts)
    {
        vertices.emplace_back(pos, m_color);
    }

    for (std::size_t i = 1; i < m_verts.size() - 1; ++i)
    {
        indices.push_back(vertexIndex + 0);
        indices.push_back(vertexIndex + i);
        indices.push_back(vertexIndex + i + 1);
    }
}

DrawSpriteBatch::DrawSpriteBatch(const gl::AbstractTexture *texture, const glm::vec4 &color, int depth)
    : DrawCommandPosTexColor(depth)
    , m_texture(texture)
    , m_color(color)
{
}

void DrawSpriteBatch::addSprite(const Vertex &topLeft, const Vertex &bottomRight)
{
    m_quads.emplace_back(topLeft, bottomRight);
}

void DrawSpriteBatch::dumpVertices(VertexPosTexColorBuffer &buffer) const
{
    auto &vertices = buffer.vertices;
    auto &indices = buffer.indices;

    auto vertexIndex = buffer.vertices.size();

    for (const auto &quad : m_quads)
    {
        const auto &topLeft = quad.topLeft;
        const auto &bottomRight = quad.bottomRight;
        vertices.emplace_back(glm::vec2{topLeft.position.x, topLeft.position.y},
                              glm::vec2{topLeft.texCoords.x, topLeft.texCoords.y}, m_color);
        vertices.emplace_back(glm::vec2{bottomRight.position.x, topLeft.position.y},
                              glm::vec2{bottomRight.texCoords.x, topLeft.texCoords.y}, m_color);
        vertices.emplace_back(glm::vec2{bottomRight.position.x, bottomRight.position.y},
                              glm::vec2{bottomRight.texCoords.x, bottomRight.texCoords.y}, m_color);
        vertices.emplace_back(glm::vec2{topLeft.position.x, bottomRight.position.y},
                              glm::vec2{topLeft.texCoords.x, bottomRight.texCoords.y}, m_color);

        indices.push_back(vertexIndex + 0);
        indices.push_back(vertexIndex + 1);
        indices.push_back(vertexIndex + 2);

        indices.push_back(vertexIndex + 2);
        indices.push_back(vertexIndex + 3);
        indices.push_back(vertexIndex + 0);

        vertexIndex += 4;
    }
}

Painter::Painter(ShaderManager *shaderManager)
    : m_shaderManager(shaderManager)
    , m_spriteBook(std::make_unique<SpriteTextureBook>(kSpriteSheetHeight, kSpriteSheetWidth))
{
}

Painter::~Painter() = default;

void Painter::setViewportSize(const SizeI &size)
{
    m_viewportSize = size;

    m_projectionMatrix = glm::ortho(0.0f, static_cast<float>(size.width()), static_cast<float>(size.height()), 0.0f);

    m_shaderManager->setCurrent(ShaderManager::Shader::Text);
    m_shaderManager->setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, m_projectionMatrix);

    m_shaderManager->setCurrent(ShaderManager::Shader::Flat);
    m_shaderManager->setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, m_projectionMatrix);
}

void Painter::begin()
{
    m_color = glm::vec4{1.0};
    m_fontMetrics.reset();
    m_glyphCache = nullptr;
    m_commands.clear();

    // TODO: restore previous scissor state
    setClipRect(RectF{glm::vec2{0.0f}, SizeF{m_viewportSize}});
}

void Painter::end()
{
    flushCommandQueue();

    // TODO: restore scissor state to what it was before begin() call
    glDisable(GL_SCISSOR_TEST);
}

void Painter::flushCommandQueue()
{
    std::ranges::sort(m_commands, [](const auto &lhs, const auto &rhs) {
        return std::tuple(lhs->depth(), lhs->vertexType(), lhs->texture()) <
               std::tuple(rhs->depth(), rhs->vertexType(), rhs->texture());
    });

    VertexPosColorBuffer vertexPosColorBuffer;
    VertexPosTexColorBuffer vertexPosTexColorBuffer;

    auto batchStart = m_commands.begin();
    while (batchStart != m_commands.end())
    {
        const auto vertexType = (*batchStart)->vertexType();
        const auto *texture = (*batchStart)->texture();
        const auto batchEnd =
            std::find_if(std::next(batchStart), m_commands.end(), [vertexType, texture](const auto &command) {
                return command->vertexType() != vertexType || command->texture() != texture;
            });
        auto fillBuffer = [batchStart, batchEnd](auto &buffer) {
            buffer.vertices.clear();
            buffer.indices.clear();
            for (auto it = batchStart; it != batchEnd; ++it)
            {
                (*it)->dumpVertices(buffer);
            }
            buffer.uploadData();
        };
        switch (vertexType)
        {
        case VertexType::PosColor: {
            fillBuffer(vertexPosColorBuffer);
            m_shaderManager->setCurrent(ShaderManager::Shader::Flat);
            vertexPosColorBuffer.draw();
            break;
        }
        case VertexType::PosTexColor: {
            fillBuffer(vertexPosTexColorBuffer);
            texture->bind();
            m_shaderManager->setCurrent(ShaderManager::Shader::Text);
            vertexPosTexColorBuffer.draw();
            break;
        }
        }
        batchStart = batchEnd;
    }

    m_commands.clear();
}

void Painter::setColor(const glm::vec4 &color)
{
    m_color = color;
}

void Painter::setFont(const Font &font)
{
    if (font == this->font())
        return;
    auto it = m_glyphCaches.find(font);
    if (it == m_glyphCaches.end())
    {
        it = m_glyphCaches.insert(it, {font, std::make_unique<GlyphCache>(font, m_spriteBook.get())});
    }
    m_glyphCache = it->second.get();
    m_fontMetrics = FontMetrics(font);
}

Font Painter::font() const
{
    return m_glyphCache != nullptr ? m_glyphCache->font() : Font{};
}

void Painter::setClipRect(const RectF &clipRect)
{
    if (clipRect == m_clipRect)
        return;

    // Temporary hack just to get this over with. Ideally we want to batch and draw everything outside the clipRect,
    // before setting the clip rect and drawing stuff inside the clip rect! How?
    // * Store clip rect in draw commands, sort by clip rect and depth?
    // * Clip draw commands on the CPU (maybe too expensive and complicated, especially for things like stroked paths)?
    // * Store a tree of draw commands, with the clip rect on each node, then instead of setClipRect we could have
    // pushClipRect/popClipRect?
    //
    //     struct Node
    //     {
    //        RectF clipRect;
    //        std::vector<std::unique_ptr<DrawCommand>> commands;
    //        std::vector<std::unique_ptr<Node>> children;
    //     };
    //
    flushCommandQueue();

    m_clipRect = clipRect;
    if (m_clipRect.isNull())
    {
        glDisable(GL_SCISSOR_TEST);
    }
    else
    {
        glScissor(m_clipRect.left(), m_viewportSize.height() - (m_clipRect.top() + m_clipRect.height()),
                  m_clipRect.width(), m_clipRect.height());
        glEnable(GL_SCISSOR_TEST);
    }
}

void Painter::drawFilledConvexPolygon(std::span<const glm::vec2> verts, int depth)
{
    m_commands.push_back(std::make_unique<DrawFilledConvexPolygon>(verts, m_color, depth));
}

void Painter::drawRect(const RectF &rect, int depth)
{
    const std::array<glm::vec2, 4> verts = {glm::vec2{rect.left(), rect.top()}, glm::vec2{rect.right(), rect.top()},
                                            glm::vec2{rect.right(), rect.bottom()},
                                            glm::vec2{rect.left(), rect.bottom()}};
    drawFilledConvexPolygon(verts, depth);
}

void Painter::drawText(const glm::vec2 &pos, const std::string_view text, int depth)
{
    if (!m_glyphCache)
        return;

    assert(m_fontMetrics.has_value());

    std::unordered_map<const gl::AbstractTexture *, std::unique_ptr<DrawSpriteBatch>> commands;

    glm::vec2 p = pos;
    for (size_t index = 0; const char ch : text)
    {
        const auto glyph = m_glyphCache->findOrCreateGlyph(ch);
        if (glyph.has_value())
        {
            DrawSpriteBatch *command = [this, &glyph, &commands, depth]() {
                auto it = commands.find(glyph->texture);
                if (it == commands.end())
                    it = commands.insert(
                        it, {glyph->texture, std::make_unique<DrawSpriteBatch>(glyph->texture, m_color, depth)});
                return it->second.get();
            }();
            command->addSprite({p + glyph->quad.topLeft(), glyph->texCoords.topLeft()},
                               {p + glyph->quad.bottomRight(), glyph->texCoords.bottomRight()});
            p += glm::vec2(glyph->advance, 0);
            if (index < text.size() - 1)
                p += glm::vec2(m_fontMetrics->kernAdvance(ch, text[index + 1]), 0.0f);
        }
        ++index;
    }

    for (auto &[_, command] : commands)
        m_commands.push_back(std::move(command));
}
