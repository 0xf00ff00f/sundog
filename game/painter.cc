#include "painter.h"

#include "tile_batcher.h"
#include "glyph_cache.h"
#include "sprite_texture_book.h"

namespace
{
constexpr auto kSpriteSheetHeight = 1024;
constexpr auto kSpriteSheetWidth = 1024;
} // namespace

Painter::Painter()
    : m_tileBatcher(std::make_unique<TileBatcher>())
    , m_spriteBook(std::make_unique<SpriteTextureBook>(kSpriteSheetHeight, kSpriteSheetWidth))
{
}

Painter::~Painter() = default;

void Painter::begin()
{
    m_fontMetrics.reset();
    m_glyphCache = nullptr;
    m_tileBatcher->reset();
}

void Painter::end()
{
    m_tileBatcher->blit();
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

void Painter::drawText(const glm::vec2 &pos, const std::string_view text, float depth)
{
    if (!m_glyphCache)
        return;

    assert(m_fontMetrics.has_value());

    glm::vec2 p = pos;
    for (size_t index = 0; const char ch : text)
    {
        const auto glyph = m_glyphCache->findOrCreateGlyph(ch);
        if (glyph.has_value())
        {
            m_tileBatcher->setTexture(glyph->texture);
            m_tileBatcher->addTile({p + glyph->quad.topLeft(), glyph->texCoords.topLeft()},
                                   {p + glyph->quad.bottomRight(), glyph->texCoords.bottomRight()});
            p += glm::vec2(glyph->advance, 0);
            if (index < text.size() - 1)
                p += glm::vec2(m_fontMetrics->kernAdvance(ch, text[index + 1]), 0.0f);
        }
        ++index;
    }
}
