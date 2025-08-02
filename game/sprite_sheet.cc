#include "sprite_sheet.h"

#include <utility>
#include <cassert>

struct SpriteSheet::Node
{
    Rect rect;
    std::unique_ptr<Node> left, right;
    bool used{false};

    Node(size_t x, size_t y, size_t width, size_t height);
    std::optional<Rect> insert(size_t width, size_t height);
};

SpriteSheet::Node::Node(size_t x, size_t y, size_t width, size_t height)
    : rect{x, y, width, height}
{
}

std::optional<Rect> SpriteSheet::Node::insert(size_t width, size_t height)
{
    if (used)
        return std::nullopt;

    if (width > rect.width || height > rect.height)
        return std::nullopt;

    // is this an internal node?
    if (left)
    {
        auto result = left->insert(width, height);
        if (!result)
        {
            assert(right);
            result = right->insert(width, height);
        }
        return result;
    }

    // image fits perfectly in this node?
    if (width == rect.width && height == rect.height)
    {
        used = true;
        return rect;
    }

    // else split this node
    const auto splitX = rect.width - width;
    const auto splitY = rect.height - height;
    if (splitX > splitY)
    {
        // split horizontally
        left = std::make_unique<Node>(rect.x, rect.y, width, rect.height);
        right = std::make_unique<Node>(rect.x + width, rect.y, splitX, rect.height);
        return left->insert(width, height);
    }
    else
    {
        // split vertically
        left = std::make_unique<Node>(rect.x, rect.y, rect.width, height);
        right = std::make_unique<Node>(rect.x, rect.y + height, rect.width, splitY);
        return left->insert(width, height);
    }
}

SpriteSheet::SpriteSheet(size_t width, size_t height)
    : m_image(width, height)
    , m_tree(std::make_unique<Node>(0, 0, width, height))
{
}

SpriteSheet::SpriteSheet(SpriteSheet &&other)
    : m_image(std::exchange(other.m_image, {}))
    , m_tree(std::move(other.m_tree))
{
}

SpriteSheet::~SpriteSheet() = default;

SpriteSheet &SpriteSheet::operator=(SpriteSheet &&other)
{
    SpriteSheet temp(std::move(other));
    swap(*this, temp);
    return *this;
}

std::optional<Rect> SpriteSheet::tryInsert(const Image<uint32_t> &image)
{
    constexpr auto Margin = 1;

    auto rect = m_tree->insert(image.width() + 2 * Margin, image.height() + 2 * Margin);
    if (!rect)
        return std::nullopt;

    const auto *src = image.pixels().data();
    const auto srcSpan = image.width();

    auto *dest = m_image.pixels().data() + (rect->y + Margin) * m_image.width() + rect->x + Margin;
    const auto destSpan = m_image.width();

    for (size_t i = 0; i < image.height(); ++i)
    {
        std::copy(src, src + srcSpan, dest);
        src += srcSpan;
        dest += destSpan;
    }

    changed();

    return rect;
}
