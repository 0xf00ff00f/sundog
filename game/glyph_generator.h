#pragma once

#include "font.h"
#include "image.h"

#include <glm/glm.hpp>

#include <stb_truetype.h>

class GlyphGenerator
{
public:
    explicit GlyphGenerator(const Font &font);
    ~GlyphGenerator();

    bool valid() const;

    const Font &font() const { return m_font; }

    struct GlyphImage
    {
        float advance{0.0f};
        glm::vec2 topLeft{0.0f};
        Image<uint32_t> image;
    };
    GlyphImage generate(char32_t codepoint) const;

    Image<uint32_t> generate(std::u32string_view text) const;

private:
    Font m_font;
    const FontInfo *m_fontInfo{nullptr};
    float m_scale{0.0f};
};
