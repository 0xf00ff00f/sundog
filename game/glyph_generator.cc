#include "glyph_generator.h"

#include "font_info.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <algorithm>
#include <cmath>
#include <mdspan>

namespace
{

constexpr auto kGlyphMargin = 1;

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

    static_assert(sizeof(glm::u8vec4) == sizeof(*image.pixels().data()));
    const auto sourcePixels = std::mdspan(reinterpret_cast<const glm::u8vec4 *>(image.pixels().data()), height, width);
    assert(sourcePixels.extent(0) == height);
    assert(sourcePixels.extent(1) == width);

    Image<uint32_t> destImage(width, height);
    auto destPixels = std::mdspan(reinterpret_cast<glm::u8vec4 *>(destImage.pixels().data()), height, width);

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
                    const auto sourceAlpha = static_cast<int>(sourcePixels[i, j].a);
                    alpha = std::max(alpha, static_cast<int>(w * sourceAlpha));
                }
            }

            const auto origAlpha = sourcePixels[y, x].a;
            const auto destColor = glm::u8vec4(origAlpha, origAlpha, origAlpha, alpha);
            destPixels[y, x] = destColor;
        }
    }

    image = std::move(destImage);
}

} // namespace

GlyphGenerator::GlyphGenerator(const Font &font)
    : m_font(font)
    , m_fontInfo(findOrCreateFontInfo(font.name))
    , m_scale(m_fontInfo->scaleForPixelHeight(font.pixelHeight))
{
}

GlyphGenerator::~GlyphGenerator() = default;

bool GlyphGenerator::valid() const
{
    return m_fontInfo->loaded();
}

GlyphGenerator::GlyphImage GlyphGenerator::generate(char32_t codepoint) const
{
    if (!m_fontInfo->loaded())
        return {};

    const auto advance = m_scale * m_fontInfo->horizontalAdvance(codepoint);
    const auto baseline = m_scale * m_fontInfo->ascent();
    const auto margin = kGlyphMargin + m_font.outlineSize;

    const auto [box, image8] = m_fontInfo->bitmap(codepoint, {m_scale, m_scale});

    Image<uint32_t> image32(image8.width() + 2 * margin, image8.height() + 2 * margin);

    const auto sourcePixels = std::mdspan(image8.pixels().data(), image8.height(), image8.width());

    static_assert(sizeof(glm::u8vec4) == sizeof(*image32.pixels().data()));
    auto destPixels =
        std::mdspan(reinterpret_cast<glm::u8vec4 *>(image32.pixels().data()), image32.height(), image32.width());

    for (size_t i = 0; i < image8.height(); ++i)
    {
        const auto *sourceRow = &sourcePixels[i, 0];
        auto *destRow = &destPixels[i + margin, margin];
        std::transform(sourceRow, sourceRow + image8.width(), destRow,
                       [](uint8_t alpha) { return glm::u8vec4(255, 255, 255, alpha); });
    }

    if (m_font.outlineSize > 0)
        dilateAlpha(image32, 2 * m_font.outlineSize + 1);

    return GlyphImage{.advance = advance,
                      .topLeft = glm::vec2{box.left() - margin, box.top() + baseline - margin},
                      .image = std::move(image32)};
}

Image<uint32_t> GlyphGenerator::generate(std::u32string_view text) const
{
    if (!m_fontInfo->loaded() || text.empty())
        return {};

    const auto margin = kGlyphMargin + m_font.outlineSize;

    // collect bitmaps

    std::vector<FontInfo::Bitmap> bitmaps;
    float xPos = 0.0f;
    for (size_t i = 0; i < text.size(); ++i)
    {
        const auto codepoint = text[i];

        const auto xShift = xPos - std::floorf(xPos);
        auto bitmap = m_fontInfo->bitmap(codepoint, {m_scale, m_scale}, {xShift, 0.0f});
        bitmap.box.setLeft(static_cast<int>(xPos) + bitmap.box.left());
        bitmaps.push_back(std::move(bitmap));

        xPos += m_scale * m_fontInfo->horizontalAdvance(codepoint);
        if (i < text.size() - 1)
            xPos += m_scale * m_fontInfo->kernAdvance(text[i], text[i + 1]);
    }

    // figure out label width

    int xMax = std::numeric_limits<int>::lowest();
    int xMin = std::numeric_limits<int>::max();
    for (const auto &[box, _] : bitmaps)
    {
        xMin = std::min(xMin, box.left());
        xMax = std::max(xMax, box.right());
    }
    const auto labelWidth = xMax - xMin;

    // render label

    const auto baseline = static_cast<int>(m_scale * m_fontInfo->ascent());

    Image<uint32_t> labelImage(labelWidth + 2 * margin, m_font.pixelHeight + 2 * margin);
    static_assert(sizeof(glm::u8vec4) == sizeof(*labelImage.pixels().data()));
    auto labelPixels = std::mdspan(reinterpret_cast<glm::u8vec4 *>(labelImage.pixels().data()), labelImage.height(),
                                   labelImage.width());

    for (const auto &[glyphBox, glyphImage] : bitmaps)
    {
        assert(glyphBox.width() == glyphImage.width());
        assert(glyphBox.height() == glyphImage.height());
        const auto glyphWidth = glyphBox.width();
        const auto glyphHeight = glyphBox.height();

        const auto left = glyphBox.left() - xMin;
        assert(left >= 0);
        assert(left + glyphWidth <= labelImage.width());
        assert(baseline + glyphBox.top() >= 0);
        assert(baseline + glyphBox.bottom() <= labelImage.height());

        // blend glyph image into the label image
        auto glyphPixels = std::mdspan(glyphImage.pixels().data(), glyphImage.height(), glyphImage.width());
        for (size_t j = 0; j < glyphHeight; ++j)
        {
            const auto *src = &glyphPixels[j, 0];
            auto *dest = &labelPixels[baseline + glyphBox.top() + j + margin, left + margin];
            for (size_t k = 0; k < glyphWidth; ++k)
            {
                const auto alpha = std::min(uint32_t(*src) + uint32_t(dest->a), uint32_t(255));
                *dest = glm::u8vec4(255, 255, 255, alpha);
                ++src;
                ++dest;
            }
        }
    }

    if (m_font.outlineSize > 0)
        dilateAlpha(labelImage, 2 * m_font.outlineSize + 1);

    return labelImage;
}
