#include "sprite_book.h"
#include "sprite_sheet.h"

#include <ranges>
#include <cassert>

SpriteBook::SpriteBook(size_t pageWidth, size_t pageHeight, size_t margin)
    : m_pageWidth(pageWidth)
    , m_pageHeight(pageHeight)
    , m_margin(margin)
{
}

SpriteBook::~SpriteBook() = default;

std::optional<SpriteBook::Entry> SpriteBook::tryInsert(const Image<uint32_t> &image)
{
    if (image.width() + 2 * m_margin > m_pageWidth || image.height() + 2 * m_margin > m_pageHeight)
        return {};

    // Can we insert it into one of the existing sheets?
    for (const auto &sheet : m_sheets)
    {
        if (auto rect = sheet->tryInsert(image); rect.has_value())
        {
            return Entry{*rect, &sheet->image()};
        }
    }

    // Create a new sheet
    m_sheets.emplace_back(std::make_unique<SpriteSheet>(m_pageWidth, m_pageHeight, m_margin));

    auto &sheet = m_sheets.back();
    if (auto rect = sheet->tryInsert(image); rect.has_value())
    {
        return Entry{*rect, &sheet->image()};
    }

    // Shouldn't happen
    assert(false);
    return {};
}

std::vector<const Image<uint32_t> *> SpriteBook::pages() const
{
    return m_sheets | std::views::transform([](const auto &sheet) { return &sheet->image(); }) |
           std::ranges::to<std::vector>();
}
