#include "font.h"

#include "font_info.h"

std::size_t std::hash<Font>::operator()(const Font &font) const
{
    return ((hash<string>()(font.name) ^ (hash<float>()(font.pixelHeight) << 1)) >> 1) ^
           (hash<int>()(font.outlineSize) << 1);
}

FontMetrics::FontMetrics(const Font &font)
    : m_font(font)
    , m_fontInfo(findOrCreateFontInfo(font.name))
    , m_scale(m_fontInfo->scaleForPixelHeight(m_font.pixelHeight))
{
}

bool FontMetrics::valid() const
{
    return m_fontInfo->loaded();
}

std::string_view FontMetrics::name() const
{
    return m_font.name;
}

float FontMetrics::pixelHeight() const
{
    return m_font.pixelHeight; // TODO: + 2 * m_outlineSize?
}

float FontMetrics::ascent() const
{
    return m_scale * m_fontInfo->ascent();
}

float FontMetrics::horizontalAdvance(char32_t codepoint) const
{
    return m_scale * m_fontInfo->horizontalAdvance(codepoint);
}

template<typename CharT>
float FontMetrics::horizontalAdvance(std::basic_string_view<CharT> text) const
{
    int advance = 0;
    for (std::size_t index = 0; const auto codepoint : text)
    {
        advance += m_fontInfo->horizontalAdvance(codepoint);
        if (index < text.size() - 1)
            advance += m_fontInfo->kernAdvance(codepoint, text[index + 1]);
        ++index;
    }
    return m_scale * advance;
}

float FontMetrics::kernAdvance(char32_t a, char32_t b) const
{
    return m_scale * m_fontInfo->kernAdvance(a, b);
}

template float FontMetrics::horizontalAdvance(std::string_view text) const;
template float FontMetrics::horizontalAdvance(std::u32string_view text) const;
