#include "texture_cache.h"

#include "asset_path.h"
#include "glhelpers.h"

TextureCache::TextureCache() = default;

TextureCache::~TextureCache() = default;

const gl::Texture *TextureCache::findOrCreateTexture(std::string_view name)
{
    auto it = m_cache.find(name);
    if (it == m_cache.end())
    {
        const auto image = loadImage(imageFilePath(name), true);
        auto texture = std::make_unique<gl::Texture>(image);
        texture->setMinificationFilter(gl::Texture::Filter::Linear);
        texture->setMagnificationFilter(gl::Texture::Filter::Linear);
        texture->setWrapModeS(gl::Texture::WrapMode::Repeat);
        texture->setWrapModeT(gl::Texture::WrapMode::Repeat);
        it = m_cache.insert(it, {std::string{name}, std::move(texture)});
    }
    return it->second.get();
}
