#pragma once

#include <string>
#include <string_view>

struct Font
{
    std::string name;
    float pixelHeight{0.0f};
    int outlineSize{0};

    bool operator==(const Font &) const = default;
};

template<>
struct std::hash<Font>
{
    std::size_t operator()(const Font &font) const;
};

class FontInfo;

class FontMetrics
{
public:
    explicit FontMetrics(const Font &font);

    bool valid() const;

    float pixelHeight();
    std::string_view name() const;
    float ascent() const;
    float horizontalAdvance(char32_t codepoint) const;
    float horizontalAdvance(std::u32string_view text) const;
    float kernAdvance(char32_t a, char32_t b) const;

private:
    Font m_font;
    const FontInfo *m_fontInfo{nullptr};
    float m_scale{1.0f};
};
