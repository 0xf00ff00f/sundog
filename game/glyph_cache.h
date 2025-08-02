#pragma once

#include "sprite_sheet.h"
#include "glyph_image_generator.h"
#include "glhelpers.h"

#include <optional>

#include <glm/glm.hpp>

class LazyTexture : public gl::AbstractTexture
{
public:
    explicit LazyTexture(const Image<uint32_t> *image);

    void markDirty();
    void bind() const override;

private:
    const Image<uint32_t> *m_image = nullptr;
    gl::Texture m_texture;
    mutable bool m_dirty = false;
};

class GlyphCache
{
public:
    explicit GlyphCache(const std::string &font, float pixelHeight, int outlineSize = 0);
    ~GlyphCache();

    float pixelHeight() const;
    float baseline() const;
    float kernAdvance(char32_t a, char32_t b) const;
    float textWidth(std::u32string_view text) const;

    struct Sprite
    {
        size_t width{0};
        size_t height{0};
        float advance{0.0f};
        glm::vec2 topLeft{0.0f};
        struct TexCoords
        {
            glm::vec2 topLeft{0.0f};
            glm::vec2 bottomRight{0.0f};
        };
        TexCoords texCoords;
        const gl::AbstractTexture *texture{nullptr};
    };
    std::optional<Sprite> getGlyph(char32_t codepoint);

private:
    std::optional<Sprite> createGlyph(char32_t codepoint);

    struct SpriteSheetTexture
    {
        SpriteSheetTexture();

        SpriteSheet spriteSheet;
        LazyTexture texture;
    };
    std::vector<std::unique_ptr<SpriteSheetTexture>> m_spriteSheets;
    GlyphImageGenerator m_glyphGenerator;
    std::unordered_map<char32_t, std::optional<Sprite>> m_glyphSprites;
};
