#pragma once

#include "sprite_book.h"
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

    struct Quad
    {
        glm::vec2 topLeft{0.0f};
        glm::vec2 bottomRight{0.0f};
    };
    struct Glyph
    {
        Quad quad;
        Quad texCoords;
        float advance{0.0f};
        const gl::AbstractTexture *texture{nullptr};
    };
    std::optional<Glyph> getGlyph(char32_t codepoint);

private:
    std::optional<Glyph> createGlyph(char32_t codepoint);

    SpriteBook m_spriteBook;
    std::unordered_map<const Image<uint32_t> *, std::unique_ptr<LazyTexture>> m_sheetTextures;
    GlyphImageGenerator m_glyphGenerator;
    std::unordered_map<char32_t, std::optional<Glyph>> m_glyphSprites;
};
