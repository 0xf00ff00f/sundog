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

GlyphCache::SpriteSheetTexture::SpriteSheetTexture()
    : spriteSheet(kSheetWidth, kSheetHeight)
    , texture(&spriteSheet.image())
{
}

GlyphCache::GlyphCache(const std::string &font, float pixelHeight, int outlineSize)
    : m_glyphGenerator(font, pixelHeight, outlineSize)
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

std::optional<GlyphCache::Sprite> GlyphCache::getGlyph(char32_t codepoint)
{
    auto it = m_glyphSprites.find(codepoint);
    if (it == m_glyphSprites.end())
        it = m_glyphSprites.insert(it, {codepoint, createGlyph(codepoint)});
    return it->second;
}

std::optional<GlyphCache::Sprite> GlyphCache::createGlyph(char32_t codepoint)
{
    const auto glyphImage = m_glyphGenerator.generate(codepoint);

    const auto &image = glyphImage.image;

    struct RectTexture
    {
        Rect rect;
        const gl::AbstractTexture *texture;
    };
    auto rectTexture = [this, &image]() -> std::optional<RectTexture> {
        // does the image fit in any of the existing sheets?
        for (const auto &spriteSheetTexture : m_spriteSheets)
        {
            if (auto rect = spriteSheetTexture->spriteSheet.tryInsert(image))
            {
                spriteSheetTexture->texture.markDirty();
                return RectTexture{*rect, &spriteSheetTexture->texture};
            }
        }

        // create a new sheet, try again
        m_spriteSheets.emplace_back(std::make_unique<SpriteSheetTexture>());
        const auto &spriteSheetTexture = m_spriteSheets.back();
        if (auto rect = spriteSheetTexture->spriteSheet.tryInsert(image))
        {
            spriteSheetTexture->texture.markDirty();
            return RectTexture{*rect, &spriteSheetTexture->texture};
        }

        // no dice
        return std::nullopt;
    }();
    if (!rectTexture)
        return std::nullopt;

    const auto toTexCoord = [this](size_t x, size_t y) {
        return glm::vec2(x, y) / glm::vec2(kSheetWidth, kSheetHeight);
    };

    const auto &rect = rectTexture->rect;
    const auto *texture = rectTexture->texture;

    return Sprite{.width = image.width(),
                  .height = image.height(),
                  .advance = glyphImage.advance,
                  .topLeft = glyphImage.topLeft,
                  .texCoords = {.topLeft = toTexCoord(rect.x, rect.y),
                                .bottomRight = toTexCoord(rect.x + rect.width, rect.y + rect.height)},
                  .texture = texture};
}
