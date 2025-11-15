#pragma once

#include "rect.h"
#include "image.h"

#include <memory>
#include <vector>
#include <optional>

class SpriteSheet;

class SpriteBook
{
public:
    explicit SpriteBook(size_t pageWidth, size_t pageHeight, size_t margin = 1);
    ~SpriteBook();

    struct Entry
    {
        RectI rect;
        const Image<uint32_t> *sheetImage;
    };
    std::optional<Entry> tryInsert(const Image<uint32_t> &image);

    size_t pageWidth() const { return m_pageWidth; }
    size_t pageHeight() const { return m_pageHeight; }
    size_t margin() const { return m_margin; }

    std::vector<const Image<uint32_t> *> pages() const;

private:
    size_t m_pageWidth;
    size_t m_pageHeight;
    size_t m_margin;
    std::vector<std::unique_ptr<SpriteSheet>> m_sheets;
};
