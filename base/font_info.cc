#include "font_info.h"

#include "file.h"
#include "dict.h"
#include "asset_path.h"

#include <print>

FontInfo::FontInfo(std::string_view name)
    : m_loaded(load(name))
{
}

bool FontInfo::load(const std::string_view name)
{
    const auto path = fontFilePath(name);
    m_fontBuffer = readFile(path);
    if (m_fontBuffer.empty())
    {
        std::println(stderr, "Failed to read font {}", path);
        return false;
    }

    const auto *fontData = reinterpret_cast<const unsigned char *>(m_fontBuffer.data());
    const auto result = stbtt_InitFont(&m_font, fontData, stbtt_GetFontOffsetForIndex(fontData, 0));
    if (result == 0)
    {
        std::println(stderr, "Failed to parse font {}", path);
        return false;
    }

    std::println("Loaded font {}", path);

    return true;
}

float FontInfo::scaleForPixelHeight(float pixelHeight) const
{
    if (!m_loaded)
        return 0;

    return stbtt_ScaleForPixelHeight(&m_font, pixelHeight);
}

int FontInfo::ascent() const
{
    if (!m_loaded)
        return 0;

    int ascent;
    stbtt_GetFontVMetrics(&m_font, &ascent, nullptr, nullptr);
    return ascent;
}

int FontInfo::horizontalAdvance(char32_t codepoint) const
{
    if (!m_loaded)
        return 0;

    int advance;
    stbtt_GetCodepointHMetrics(&m_font, codepoint, &advance, nullptr);
    return advance;
}

FontInfo::Bitmap FontInfo::bitmap(char32_t codepoint, const glm::vec2 &scale, const glm::vec2 &shift) const
{
    if (!m_loaded)
        return {};

    int x0, y0, x1, y1;
    stbtt_GetCodepointBitmapBoxSubpixel(&m_font, codepoint, scale.x, scale.y, shift.x, shift.y, &x0, &y0, &x1, &y1);
    const auto box = RectI(RectI::Point(x0, y0), RectI::Point(x1, y1));

    auto image = Image<uint8_t>(box.width(), box.height());
    stbtt_MakeCodepointBitmapSubpixel(&m_font, image.pixels().data(), image.width(), image.height(), image.width(),
                                      scale.x, scale.y, shift.x, shift.y, codepoint);

    return {box, std::move(image)};
}

int FontInfo::kernAdvance(char32_t a, char32_t b) const
{
    if (!m_loaded)
        return 0;

    return stbtt_GetCodepointKernAdvance(&m_font, a, b);
}

FontInfo *findOrCreateFontInfo(std::string_view name)
{
    static Dict<std::unique_ptr<FontInfo>> cache;
    auto it = cache.find(name);
    if (it == cache.end())
    {
        auto font = std::make_unique<FontInfo>(name);
        it = cache.insert(it, {std::string{name}, std::move(font)});
    }
    return it->second.get();
}
