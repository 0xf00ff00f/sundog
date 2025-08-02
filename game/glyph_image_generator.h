#pragma once

#include "image.h"

#include <string>

#include <glm/glm.hpp>

#include <stb_truetype.h>

class GlyphImageGenerator
{
public:
    explicit GlyphImageGenerator(const std::string &font, float pixelHeight, int outlineSize = 0);
    ~GlyphImageGenerator();

    bool isValid() const { return m_initialized; }

    float pixelHeight() const { return m_pixelHeight; }
    float baseline() const { return m_baseline; }
    float textWidth(std::u32string_view text) const;

    struct GlyphImage
    {
        float advance{0.0f};
        glm::vec2 topLeft{0.0f};
        Image<uint32_t> image;
    };
    GlyphImage generate(char32_t codepoint) const;

    Image<uint32_t> generate(std::u32string_view text) const;

    float kernAdvance(char32_t a, char32_t b) const;

private:
    bool initialize(const std::string &font, float pixelHeight, int outlineSize);

    std::vector<std::byte> m_fontBuffer;
    stbtt_fontinfo m_font;
    float m_pixelHeight{0.0f};
    float m_scale{0.0f};
    int m_baseline{0};
    int m_outlineSize{0};
    bool m_initialized{false};
};
