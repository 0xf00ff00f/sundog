#pragma once

#include "rect.h"
#include "image.h"
#include "sprite_book.h"

namespace gl
{
class AbstractTexture;
};

struct SpriteSheetTexture;

class SpriteTextureBook
{
public:
    explicit SpriteTextureBook(size_t textureWidth, size_t textureHeight, size_t margin = 1);
    ~SpriteTextureBook();

    struct Entry
    {
        RectF texCoords;
        SizeI size;
        const gl::AbstractTexture *texture{nullptr};
    };
    std::optional<Entry> tryInsert(const Image<uint32_t> &image);

    size_t textureWidth() const;
    size_t textureHeight() const;
    size_t margin() const;

private:
    SpriteBook m_spriteBook;
    std::unordered_map<const Image<uint32_t> *, std::unique_ptr<SpriteSheetTexture>> m_sheetTextures;
};
