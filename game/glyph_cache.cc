#include "glyph_cache.h"

namespace
{
constexpr auto kSheetWidth = 1024;
constexpr auto kSheetHeight = 1024;
} // namespace

LazyTexture::LazyTexture(const Image<uint32_t> *image)
    : m_image(image)
    , m_texture(image->width(), image->height())
{
    m_texture.setMinificationFilter(gl::Texture::Filter::Linear);
    m_texture.setMagnificationFilter(gl::Texture::Filter::Linear);
    m_texture.setWrapModeS(gl::Texture::WrapMode::Repeat);
    m_texture.setWrapModeT(gl::Texture::WrapMode::Repeat);
}

void LazyTexture::markDirty()
{
    m_dirty = true;
}

void LazyTexture::bind() const
{
    if (m_dirty)
    {
        const auto &pixels = m_image->pixels();
        m_texture.data(std::as_bytes(pixels));
        m_dirty = false;
    }
    m_texture.bind();
}

GlyphCache::GlyphCache(const std::string &font, float pixelHeight, int outlineSize)
    : m_glyphGenerator(font, pixelHeight, outlineSize)
    , m_spriteBook(kSheetWidth, kSheetHeight)
{
}

GlyphCache::~GlyphCache() = default;

float GlyphCache::pixelHeight() const
{
    return m_glyphGenerator.pixelHeight();
}

float GlyphCache::baseline() const
{
    return m_glyphGenerator.baseline();
}

float GlyphCache::kernAdvance(char32_t a, char32_t b) const
{
    return m_glyphGenerator.kernAdvance(a, b);
}

float GlyphCache::textWidth(std::u32string_view text) const
{
    return m_glyphGenerator.textWidth(text);
}

std::optional<GlyphCache::Glyph> GlyphCache::getGlyph(char32_t codepoint)
{
    auto it = m_glyphSprites.find(codepoint);
    if (it == m_glyphSprites.end())
        it = m_glyphSprites.insert(it, {codepoint, createGlyph(codepoint)});
    return it->second;
}

std::optional<GlyphCache::Glyph> GlyphCache::createGlyph(char32_t codepoint)
{
    const auto glyphImage = m_glyphGenerator.generate(codepoint);

    const auto &image = glyphImage.image;
    auto sprite = m_spriteBook.tryInsert(image);
    if (!sprite.has_value())
        return {};

    const auto toTexCoord = [this](const glm::vec2 &p) { return p / glm::vec2(kSheetWidth, kSheetHeight); };

    const auto &rect = sprite->rect;
    auto *texture = [this, sheetImage = sprite->sheetImage]() -> LazyTexture * {
        auto it = m_sheetTextures.find(sheetImage);
        if (it == m_sheetTextures.end())
        {
            it = m_sheetTextures.insert(it, {sheetImage, std::make_unique<LazyTexture>(sheetImage)});
        }
        return it->second.get();
    }();
    texture->markDirty();

    return Glyph{.quad = RectF(glm::vec2(glyphImage.topLeft), SizeF(rect.size())),
                 .texCoords = RectF(toTexCoord(rect.topLeft()), toTexCoord(rect.bottomRight())),
                 .advance = glyphImage.advance,
                 .texture = texture};
}
