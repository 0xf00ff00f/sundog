#include "glyph_image_generator.h"

#include "file.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <algorithm>
#include <cmath>
#include <print>

namespace
{

void dilateAlpha(Image<uint32_t> &image, int filterSize)
{
    assert((filterSize & 1) == 1);
    const auto halfFilterSize = filterSize / 2;

    std::vector<std::vector<float>> weightMatrix(filterSize);
    for (int i = 0; i < filterSize; ++i)
    {
        auto &row = weightMatrix[i];
        row.resize(filterSize);
        for (int j = 0; j < filterSize; ++j)
        {
            const auto dx = static_cast<float>(i - halfFilterSize);
            const auto dy = static_cast<float>(j - halfFilterSize);
            const auto d = sqrtf(dx * dx + dy * dy);
            float weight;
            if (d < halfFilterSize)
            {
                weight = 1.0f;
            }
            else if (d < halfFilterSize + 1)
            {
                weight = 1.0f - (d - halfFilterSize);
            }
            else
            {
                weight = 0.0f;
            }
            row[j] = weight;
        }
    }

    const auto width = static_cast<int>(image.width());
    const auto height = static_cast<int>(image.height());

    const auto *sourcePixels = reinterpret_cast<const glm::u8vec4 *>(image.pixels().data());

    Image<uint32_t> destImage(width, height);
    auto *destPixels = reinterpret_cast<glm::u8vec4 *>(destImage.pixels().data());

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int alpha = 0;
            for (int i = std::max(y - halfFilterSize, 0); i <= std::min(y + halfFilterSize, height - 1); ++i)
            {
                for (int j = std::max(x - halfFilterSize, 0); j <= std::min(x + halfFilterSize, width - 1); ++j)
                {
                    const auto w = weightMatrix[j - x + halfFilterSize][i - y + halfFilterSize];
                    const auto sourceAlpha = static_cast<int>(sourcePixels[i * width + j].a);
                    alpha = std::max(alpha, static_cast<int>(w * sourceAlpha));
                }
            }

            const auto origAlpha = sourcePixels[y * width + x].a;
            const auto destColor = glm::u8vec4(origAlpha, origAlpha, origAlpha, alpha);
            destPixels[y * width + x] = destColor;
        }
    }

    image = destImage;
}

} // namespace

GlyphImageGenerator::GlyphImageGenerator(const std::string &font, float pixelHeight, int outlineSize)
    : m_initialized(initialize(font, pixelHeight, outlineSize))
{
}

GlyphImageGenerator::~GlyphImageGenerator() = default;

GlyphImageGenerator::GlyphImage GlyphImageGenerator::generate(char32_t codepoint) const
{
    if (!m_initialized)
        return {};

    int advance, lsb;
    stbtt_GetCodepointHMetrics(&m_font, codepoint, &advance, &lsb);

    int x0, y0, x1, y1;
    stbtt_GetCodepointBitmapBox(&m_font, codepoint, m_scale, m_scale, &x0, &y0, &x1, &y1);

    constexpr auto Border = 1;
    const auto margin = Border + m_outlineSize;
    const auto width = x1 - x0;
    const auto height = y1 - y0;
    Image<uint8_t> image8(width + 2 * margin, height + 2 * margin);

    auto pixels8 = image8.pixels();
    const auto stride = image8.width();
    stbtt_MakeCodepointBitmap(&m_font, pixels8.data() + stride * margin + margin, width, height, stride, m_scale,
                              m_scale, codepoint);

    Image<uint32_t> image32(image8.width(), image8.height());

    static_assert(sizeof(glm::u8vec4) == sizeof(*image32.pixels().data()));
    auto *destPixels = reinterpret_cast<glm::u8vec4 *>(image32.pixels().data());
    std::transform(pixels8.begin(), pixels8.end(), destPixels,
                   [](uint8_t alpha) { return glm::u8vec4(255, 255, 255, alpha); });

    if (m_outlineSize > 0)
        dilateAlpha(image32, 2 * m_outlineSize + 1);

    return GlyphImage{.advance = advance * m_scale,
                      .topLeft = glm::vec2{x0 - margin, y0 + m_baseline - margin},
                      .image = std::move(image32)};
}

Image<uint32_t> GlyphImageGenerator::generate(std::u32string_view text) const
{
    if (!m_initialized || text.empty())
        return {};

    constexpr auto Border = 1;
    const auto margin = Border + m_outlineSize;

    // figure out label width

    int xMax = std::numeric_limits<int>::lowest();
    int xMin = std::numeric_limits<int>::max();
    float xPos = 0.0f;
    for (size_t i = 0; i < text.size(); ++i)
    {
        const auto ch = text[i];

        int advance, lsb;
        stbtt_GetCodepointHMetrics(&m_font, ch, &advance, &lsb);

        const auto xShift = xPos - floorf(xPos);
        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBoxSubpixel(&m_font, ch, m_scale, m_scale, xShift, 0, &x0, &y0, &x1, &y1);

        xMin = std::min(xMin, static_cast<int>(xPos + x0));
        xMax = std::max(xMax, static_cast<int>(xPos + x0) + (x1 - x0));

        xPos += advance * m_scale;
        if (i < text.size() - 1)
            xPos += m_scale * stbtt_GetCodepointKernAdvance(&m_font, text[i], text[i + 1]);
    }
    const auto labelWidth = xMax - xMin;

    // render label

    Image<uint32_t> labelImage(labelWidth + 2 * margin, m_pixelHeight + 2 * margin);
    const auto stride = labelImage.width();
    auto labelPixels = labelImage.pixels();

    xPos = -xMin;
    for (size_t i = 0; i < text.size(); ++i)
    {
        const auto ch = text[i];

        int advance, lsb;
        stbtt_GetCodepointHMetrics(&m_font, ch, &advance, &lsb);

        const auto xShift = xPos - floorf(xPos);
        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBoxSubpixel(&m_font, ch, m_scale, m_scale, xShift, 0, &x0, &y0, &x1, &y1);
        const auto glyphWidth = x1 - x0;
        const auto glyphHeight = y1 - y0;

        const auto left = static_cast<int>(xPos + x0);

        assert(left >= 0);
        assert(left + glyphWidth <= labelImage.width());
        assert(m_baseline + y0 >= 0);
        assert(m_baseline + y1 <= labelImage.height());

        // write glyph to a temporary buffer
        Image<uint8_t> glyphImage(glyphWidth, glyphHeight);
        auto glyphPixels = glyphImage.pixels();
        stbtt_MakeCodepointBitmapSubpixel(&m_font, glyphPixels.data(), glyphImage.width(), glyphImage.height(),
                                          glyphImage.width(), m_scale, m_scale, xShift, 0, ch);

        // blend glyph into the label buffer
        for (size_t j = 0; j < glyphHeight; ++j)
        {
            const auto *src = glyphPixels.data() + j * glyphImage.width();
            static_assert(sizeof(glm::u8vec4) == sizeof(*labelPixels.data()));
            auto *dest = reinterpret_cast<glm::u8vec4 *>(labelPixels.data() + (m_baseline + y0 + j + margin) * stride +
                                                         left + margin);
            for (size_t k = 0; k < glyphWidth; ++k)
            {
                const auto alpha = std::min(uint32_t(*src) + uint32_t(dest->a), uint32_t(255));
                *dest = glm::u8vec4(255, 255, 255, alpha);
                ++src;
                ++dest;
            }
        }

        xPos += advance * m_scale;
        if (i < text.size() - 1)
            xPos += m_scale * stbtt_GetCodepointKernAdvance(&m_font, text[i], text[i + 1]);
    }

    if (m_outlineSize > 0)
        dilateAlpha(labelImage, 2 * m_outlineSize + 1);

    return labelImage;
}

float GlyphImageGenerator::kernAdvance(char32_t a, char32_t b) const
{
    return m_scale * stbtt_GetCodepointKernAdvance(&m_font, a, b);
}

float GlyphImageGenerator::textWidth(std::u32string_view text) const
{
    float width = 0.0f;
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&m_font, text[i], &advance, &lsb);
        width += m_scale * advance;
        if (i < text.size() - 1)
            width += m_scale * stbtt_GetCodepointKernAdvance(&m_font, text[i], text[i + 1]);
    }
    return width;
}

bool GlyphImageGenerator::initialize(const std::string &font, float pixelHeight, int outlineSize)
{
    m_fontBuffer = readFile(font);
    if (m_fontBuffer.empty())
    {
        std::println(stderr, "Failed to read font {}", font);
        return false;
    }

    const auto *fontData = reinterpret_cast<const unsigned char *>(m_fontBuffer.data());
    int result = stbtt_InitFont(&m_font, fontData, stbtt_GetFontOffsetForIndex(fontData, 0));
    if (result == 0)
    {
        std::println(stderr, "Failed to parse font {}", font);
        return false;
    }

    std::println("Loaded font {}", font);

    m_pixelHeight = pixelHeight;

    m_scale = stbtt_ScaleForPixelHeight(&m_font, pixelHeight);

    int ascent;
    stbtt_GetFontVMetrics(&m_font, &ascent, 0, 0);
    m_baseline = static_cast<int>(ascent * m_scale);

    m_outlineSize = outlineSize;

    return true;
}
