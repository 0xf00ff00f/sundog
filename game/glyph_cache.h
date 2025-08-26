#pragma once

#include "sprite_texture_book.h"
#include "glyph_generator.h"

#include <optional>

class SpriteTextureBook;

class GlyphCache
{
public:
    explicit GlyphCache(const Font &font, SpriteTextureBook *spriteBook);
    ~GlyphCache();

    const Font &font() const;

    struct Glyph
    {
        RectF quad;
        RectF texCoords;
        float advance{0.0f};
        const gl::AbstractTexture *texture{nullptr};
    };
    std::optional<Glyph> findOrCreateGlyph(char32_t codepoint);

private:
    std::optional<Glyph> createGlyph(char32_t codepoint);

    SpriteTextureBook *m_spriteBook{nullptr};
    GlyphGenerator m_glyphGenerator;
    std::unordered_map<char32_t, std::optional<Glyph>> m_glyphSprites;
};
