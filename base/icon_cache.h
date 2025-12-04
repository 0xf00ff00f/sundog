#pragma once

#include "sprite_texture_book.h"

class IconCache
{
public:
    explicit IconCache(SpriteTextureBook *spriteBook);
    ~IconCache();

    struct Icon
    {
        SizeI size;
        RectF texCoords;
        const gl::AbstractTexture *texture{nullptr};
    };
    std::optional<Icon> findOrCreateIcon(std::string_view name);

private:
    SpriteTextureBook *m_spriteBook{nullptr};
    std::unordered_map<std::string, std::optional<Icon>> m_entries;
};
