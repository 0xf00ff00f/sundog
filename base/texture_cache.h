#pragma once

#include "dict.h"

namespace gl
{
class Texture;
};

class TextureCache
{
public:
    TextureCache();
    ~TextureCache();

    const gl::Texture *findOrCreateTexture(std::string_view name);

private:
    Dict<std::unique_ptr<gl::Texture>> m_cache;
};
