#pragma once

#include "rect.h"

#include <vector>
#include <span>

template<typename PixelT>
class Image
{
public:
    Image() = default;

    Image(size_t width, size_t height)
        : m_width(width)
        , m_height(height)
        , m_pixels(width * height, 0)
    {
    }

    Image(Image &&other)
        : m_width(std::exchange(other.m_width, 0))
        , m_height(std::exchange(other.m_height, 0))
        , m_pixels(std::move(other.m_pixels))
    {
    }

    Image &operator=(Image &&other)
    {
        if (this != &other)
        {
            m_width = std::exchange(other.m_width, 0);
            m_height = std::exchange(other.m_height, 0);
            m_pixels = std::move(other.m_pixels);
        }
        return *this;
    }

    Image(const Image &) = delete;
    Image &operator=(const Image &) = delete;

    size_t width() const { return m_width; }
    size_t height() const { return m_height; }
    SizeI size() const { return SizeI{static_cast<int>(m_width), static_cast<int>(m_height)}; }
    std::span<PixelT> pixels() { return m_pixels; }
    std::span<const PixelT> pixels() const { return m_pixels; }

    bool isNull() const { return m_width == 0 || m_height == 0; }

private:
    size_t m_width{0};
    size_t m_height{0};
    std::vector<PixelT> m_pixels;
};

using Image32 = Image<uint32_t>;

Image32 loadImage(const std::string &path, bool flip = false);

const Image32 *findOrCreateImage(std::string_view name);
