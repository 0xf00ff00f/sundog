#pragma once

#include "rect.h"
#include "font.h"

#include <glm/glm.hpp>

#include <string_view>
#include <memory>
#include <span>

class GlyphCache;
class SpriteTextureBook;
class IconCache;
class DrawCommand;

namespace gl
{
class AbstractTexture;
};

class Painter
{
public:
    struct CornerRadii
    {
        float topLeft;
        float topRight;
        float bottomRight;
        float bottomLeft;
    };

    enum class Rotation
    {
        Rotate0,
        Rotate90,
        Rotate180,
        Rotate270
    };

    Painter();
    ~Painter();

    void setViewportSize(const SizeI &size);

    void begin();
    void end();

    void setColor(const glm::vec4 &color);
    glm::vec4 color() const { return m_color; }

    void setFont(const Font &font);
    Font font() const;

    void setClipRect(const RectF &clipRect);
    RectF clipRect() const { return m_clipRect; }

    void strokePolyline(std::span<const glm::vec2> verts, float thickness, bool closed, int depth = 0);
    void strokeLine(const glm::vec2 &from, const glm::vec2 &to, float thickness, bool closed, int depth = 0);
    void fillConvexPolygon(std::span<const glm::vec2> verts, int depth = 0);
    void fillRect(const RectF &rect, int depth = 0);
    void strokeRect(const RectF &rect, float thickness, int depth = 0);
    void fillRoundedRect(const RectF &rect, float radius, int depth = 0);
    void fillRoundedRect(const RectF &rect, const CornerRadii &radii, int depth = 0);
    void strokeRoundedRect(const RectF &rect, float radius, float thickness, int depth = 0);
    void strokeRoundedRect(const RectF &rect, const CornerRadii &radii, float thickness, int depth = 0);
    template<typename CharT>
    void drawText(const glm::vec2 &pos, std::basic_string_view<CharT> text, Rotation rotation, int depth = 0);
    template<typename CharT>
    void drawText(const glm::vec2 &pos, std::basic_string_view<CharT> text, int depth = 0)
    {
        drawText(pos, text, Rotation::Rotate0, depth);
    }
    template<typename CharT>
    void drawText(const glm::vec2 &pos, const std::basic_string<CharT> &text, Rotation rotation, int depth = 0)
    {
        drawText(pos, std::basic_string_view<CharT>{text}, rotation, depth);
    }
    template<typename CharT>
    void drawText(const glm::vec2 &pos, const std::basic_string<CharT> &text, int depth = 0)
    {
        drawText(pos, std::basic_string_view<CharT>{text}, depth);
    }
    void drawIcon(const glm::vec2 &pos, std::string_view name, int depth = 0);
    void drawSprite(const gl::AbstractTexture *texture, const glm::vec2 &topLeft, const glm::vec2 &texCoordTopLeft,
                    const glm::vec2 &bottomRight, const glm::vec2 &texCoordBottomRight, int depth = 0);

private:
    void flushCommandQueue();

    SizeI m_viewportSize;

    std::vector<std::unique_ptr<DrawCommand>> m_commands;
    glm::vec4 m_color = glm::vec4{1.0};
    RectF m_clipRect;
    std::optional<FontMetrics> m_fontMetrics;
    std::unique_ptr<SpriteTextureBook> m_spriteBook;
    std::unordered_map<Font, std::unique_ptr<GlyphCache>> m_glyphCaches;
    std::unique_ptr<IconCache> m_iconCache;
    GlyphCache *m_glyphCache{nullptr};
    glm::mat4 m_projectionMatrix;
};
