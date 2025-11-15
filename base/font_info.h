#pragma once

#include "rect.h"
#include "image.h"

#include <stb_truetype.h>

class FontInfo
{
public:
    struct Bitmap
    {
        RectI box;
        Image<uint8_t> image;
    };

    explicit FontInfo(std::string_view name);

    bool loaded() const { return m_loaded; }

    float scaleForPixelHeight(float pixelHeight) const;
    int ascent() const;
    int horizontalAdvance(char32_t codepoint) const;
    Bitmap bitmap(char32_t codepoint, const glm::vec2 &scale, const glm::vec2 &shift = {0.0f, 0.0f}) const;
    int kernAdvance(char32_t a, char32_t b) const;

private:
    bool load(std::string_view name);

    std::vector<std::byte> m_fontBuffer;
    stbtt_fontinfo m_font;
    bool m_loaded{false};
};

FontInfo *findOrCreateFontInfo(std::string_view name);
