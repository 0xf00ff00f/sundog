#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <concepts>

template<typename T>
struct Size
{
public:
    explicit Size(T width, T height)
        : m_width{width}
        , m_height{height}
    {
    }

    template<std::convertible_to<T> U>
    explicit Size(const Size<U> &other)
        : m_width(other.width())
        , m_height(other.height())
    {
    }

    T width() const { return m_width; }

    T height() const { return m_height; }

private:
    T m_width;
    T m_height;
};

using SizeI = Size<int>;
using SizeF = Size<float>;

template<typename T>
class Rect
{
public:
    using Point = glm::vec<2, T>;
    using Size = Size<T>;

    explicit Rect(T x, T y, T width, T height)
        : m_topLeft{x, y}
        , m_size{width, height}
    {
    }

    explicit Rect(const Point &position, const Size &size)
        : m_topLeft{position}
        , m_size{size}
    {
    }

    explicit Rect(const Point &topLeft, const Point &bottomRight)
        : m_topLeft{topLeft}
        , m_size{bottomRight.x - topLeft.x, bottomRight.y - topLeft.y}
    {
    }

    template<std::convertible_to<T> U>
    explicit Rect(const U &other)
        : m_topLeft(other.topLeft())
        , m_size(other.size())
    {
    }

    T width() const { return m_size.width(); }

    T height() const { return m_size.height(); }

    Size size() const { return m_size; }

    Point topLeft() const { return m_topLeft; }

    Point bottomRight() const { return m_topLeft + Point{m_size.width(), m_size.height()}; }

    T left() const { return m_topLeft.x; }

    T right() const { return m_topLeft.x + m_size.width(); }

    T top() const { return m_topLeft.y; }

    T bottom() const { return m_topLeft.y + m_size.height(); }

private:
    Point m_topLeft;
    Size m_size;
};

using RectI = Rect<int>;
using RectF = Rect<float>;
