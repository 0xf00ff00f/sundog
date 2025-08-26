#pragma once

#include "font.h"

#include <glm/glm.hpp>

#include <string_view>
#include <memory>

class TileBatcher;
class GlyphCache;
class SpriteTextureBook;

using Color = glm::vec4;

class Painter
{
public:
    Painter();
    ~Painter();

    void begin();
    void end();

    void setFont(const Font &font);
    Font font() const;

    void drawText(const glm::vec2 &pos, const std::string_view text, float depth = 0.0f);

private:
    Font m_font;
    std::optional<FontMetrics> m_fontMetrics;

    std::unique_ptr<TileBatcher> m_tileBatcher;
    std::unique_ptr<SpriteTextureBook> m_spriteBook;
    std::unordered_map<Font, std::unique_ptr<GlyphCache>> m_glyphCaches;
    GlyphCache *m_glyphCache{nullptr};
};
