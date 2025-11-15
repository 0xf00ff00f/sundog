#include "glyph_cache.h"

GlyphCache::GlyphCache(const Font &font, SpriteTextureBook *spriteBook)
    : m_glyphGenerator(font)
    , m_spriteBook(spriteBook)
{
}

GlyphCache::~GlyphCache() = default;

const Font &GlyphCache::font() const
{
    return m_glyphGenerator.font();
}

std::optional<GlyphCache::Glyph> GlyphCache::findOrCreateGlyph(char32_t codepoint)
{
    auto it = m_glyphSprites.find(codepoint);
    if (it == m_glyphSprites.end())
        it = m_glyphSprites.insert(it, {codepoint, createGlyph(codepoint)});
    return it->second;
}

std::optional<GlyphCache::Glyph> GlyphCache::createGlyph(char32_t codepoint)
{
    if (!m_glyphGenerator.valid())
        return {};

    const auto glyphImage = m_glyphGenerator.generate(codepoint);

    const auto &image = glyphImage.image;
    auto sprite = m_spriteBook->tryInsert(image);
    if (!sprite.has_value())
        return {};
    return Glyph{.quad =
                     RectF(glm::vec2(glyphImage.topLeft) - glm::vec2(m_spriteBook->margin(), m_spriteBook->margin()),
                           SizeF(sprite->size)),
                 .texCoords = sprite->texCoords,
                 .advance = glyphImage.advance,
                 .texture = sprite->texture};
}
