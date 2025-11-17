#pragma once

#include "rect.h"
#include "font.h"

#include <glm/glm.hpp>

#include <string_view>
#include <memory>
#include <span>

class ShaderManager;
class GlyphCache;
class SpriteTextureBook;
class DrawCommand;

class Painter
{
public:
    explicit Painter(ShaderManager *shaderManager);
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

    void drawPolyline(std::span<const glm::vec2> verts, float thickness, bool closed, int depth = 0);
    void drawFilledConvexPolygon(std::span<const glm::vec2> verts, int depth = 0);
    void drawText(const glm::vec2 &pos, const std::string_view text, int depth = 0);

private:
    void flushCommandQueue();

    ShaderManager *m_shaderManager{nullptr};
    SizeI m_viewportSize;

    std::vector<std::unique_ptr<DrawCommand>> m_commands;
    glm::vec4 m_color = glm::vec4{1.0};
    RectF m_clipRect;
    std::optional<FontMetrics> m_fontMetrics;
    std::unique_ptr<SpriteTextureBook> m_spriteBook;
    std::unordered_map<Font, std::unique_ptr<GlyphCache>> m_glyphCaches;
    GlyphCache *m_glyphCache{nullptr};
    glm::mat4 m_projectionMatrix;
};
