#pragma once

#include "image.h"

#include <muslots/muslots.h>

#include <optional>
#include <memory>

struct Rect
{
    size_t x{0};
    size_t y{0};
    size_t width{0};
    size_t height{0};
};

class SpriteSheet
{
public:
    SpriteSheet(size_t width, size_t height);
    ~SpriteSheet();

    // not copyable
    SpriteSheet(const SpriteSheet &) = delete;
    SpriteSheet &operator=(const SpriteSheet &) = delete;

    // movable
    SpriteSheet(SpriteSheet &&other);
    SpriteSheet &operator=(SpriteSheet &&other);

    friend inline void swap(SpriteSheet &lhs, SpriteSheet &rhs)
    {
        using std::swap;
        swap(lhs.m_image, rhs.m_image);
        swap(lhs.m_tree, rhs.m_tree);
    }

    std::optional<Rect> tryInsert(const Image<uint32_t> &image);

    size_t width() const { return m_image.width(); }
    size_t height() const { return m_image.height(); }
    const Image<uint32_t> &image() const { return m_image; }

    muslots::Signal<> changed;

private:
    Image<uint32_t> m_image;
    struct Node;
    std::unique_ptr<Node> m_tree;
};
