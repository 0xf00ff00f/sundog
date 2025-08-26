#include "sprite_texture_book.h"

#include "glhelpers.h"

class SpriteSheetTexture : public gl::AbstractTexture
{
public:
    explicit SpriteSheetTexture(const Image<uint32_t> *image);

    void markDirty();
    void bind() const override;

private:
    const Image<uint32_t> *m_image = nullptr;
    gl::Texture m_texture;
    mutable bool m_dirty = false;
};

SpriteSheetTexture::SpriteSheetTexture(const Image<uint32_t> *image)
    : m_image(image)
    , m_texture(image->width(), image->height())
{
    m_texture.setMinificationFilter(gl::Texture::Filter::Linear);
    m_texture.setMagnificationFilter(gl::Texture::Filter::Linear);
    m_texture.setWrapModeS(gl::Texture::WrapMode::Repeat);
    m_texture.setWrapModeT(gl::Texture::WrapMode::Repeat);
}

void SpriteSheetTexture::markDirty()
{
    m_dirty = true;
}

void SpriteSheetTexture::bind() const
{
    if (m_dirty)
    {
        const auto &pixels = m_image->pixels();
        m_texture.data(std::as_bytes(pixels));
        m_dirty = false;
    }
    m_texture.bind();
}

SpriteTextureBook::SpriteTextureBook(size_t textureWidth, size_t textureHeight, size_t margin)
    : m_spriteBook(textureWidth, textureHeight, margin)
{
}

SpriteTextureBook::~SpriteTextureBook() = default;

size_t SpriteTextureBook::textureWidth() const
{
    return m_spriteBook.pageWidth();
}

size_t SpriteTextureBook::textureHeight() const
{
    return m_spriteBook.pageHeight();
}

size_t SpriteTextureBook::margin() const
{
    return m_spriteBook.margin();
}

std::optional<SpriteTextureBook::Entry> SpriteTextureBook::tryInsert(const Image<uint32_t> &image)
{
    auto sprite = m_spriteBook.tryInsert(image);
    if (!sprite)
        return {};

    auto *texture = [this, sheetImage = sprite->sheetImage]() -> SpriteSheetTexture * {
        auto it = m_sheetTextures.find(sheetImage);
        if (it == m_sheetTextures.end())
        {
            it = m_sheetTextures.insert(it, {sheetImage, std::make_unique<SpriteSheetTexture>(sheetImage)});
        }
        return it->second.get();
    }();
    texture->markDirty();

    const auto toTexCoord = [this](const glm::vec2 &p) {
        return p / glm::vec2(m_spriteBook.pageWidth(), m_spriteBook.pageHeight());
    };
    const auto &rect = sprite->rect;
    const RectF texCoords(toTexCoord(rect.topLeft()), toTexCoord(rect.bottomRight()));

    return Entry{.texCoords = texCoords, .size = rect.size(), .texture = texture};
}
