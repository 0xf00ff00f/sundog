#include "icon_cache.h"

IconCache::IconCache(SpriteTextureBook *spriteBook)
    : m_spriteBook(spriteBook)
{
}

IconCache::~IconCache() = default;

std::optional<IconCache::Icon> IconCache::findOrCreateIcon(std::string_view name)
{
    auto it = m_entries.find(name);
    if (it == m_entries.end())
    {
        const auto entry = [this, &name]() -> Icon {
            const auto *image = findOrCreateImage(name);
            if (!image)
                return {};
            auto sprite = m_spriteBook->tryInsert(*image);
            if (!sprite.has_value())
                return {};
            return Icon{.size = sprite->size, .texCoords = sprite->texCoords, .texture = sprite->texture};
        }();
        it = m_entries.insert(it, {std::string{name}, entry});
    }
    return it->second;
}
