#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <concepts>
#include <format>

template<typename T>
struct Size
{
public:
    constexpr Size() = default;

    constexpr explicit Size(T width, T height)
        : m_width{width}
        , m_height{height}
    {
    }

    template<std::convertible_to<T> U>
    constexpr explicit Size(const Size<U> &other)
        : m_width(other.width())
        , m_height(other.height())
    {
    }

    constexpr T width() const { return m_width; }
    constexpr void setWidth(float width) { m_width = width; }

    constexpr T height() const { return m_height; }
    constexpr void setHeight(float height) { m_height = height; }

    constexpr bool isNull() const { return m_width == 0 || m_height == 0; }

    constexpr bool operator==(const Size &other) const = default;

private:
    T m_width{0};
    T m_height{0};
};

using SizeI = Size<int>;
using SizeF = Size<float>;

template<typename T>
struct std::formatter<Size<T>> : std::formatter<std::string_view>
{
    constexpr auto format(const Size<T> &size, std::format_context &ctx) const
    {
        auto s = std::format("Size({}x{})", size.width(), size.height());
        return std::formatter<std::string_view>::format(s, ctx);
    }
};

template<typename T>
class Rect
{
public:
    using Point = glm::vec<2, T>;
    using Size = Size<T>;

    constexpr Rect() = default;

    constexpr explicit Rect(T x, T y, T width, T height)
        : m_topLeft{x, y}
        , m_size{width, height}
    {
    }

    constexpr explicit Rect(const Point &position, const Size &size)
        : m_topLeft{position}
        , m_size{size}
    {
    }

    constexpr explicit Rect(const Point &topLeft, const Point &bottomRight)
        : m_topLeft{topLeft}
        , m_size{bottomRight.x - topLeft.x, bottomRight.y - topLeft.y}
    {
    }

    template<std::convertible_to<T> U>
    constexpr explicit Rect(const U &other)
        : m_topLeft(other.topLeft())
        , m_size(other.size())
    {
    }

    constexpr T width() const { return m_size.width(); }
    constexpr T height() const { return m_size.height(); }
    constexpr Size size() const { return m_size; }

    constexpr Point topLeft() const { return m_topLeft; }
    constexpr Point bottomRight() const { return m_topLeft + Point{m_size.width(), m_size.height()}; }

    constexpr T left() const { return m_topLeft.x; }
    constexpr T right() const { return m_topLeft.x + m_size.width(); }
    constexpr T top() const { return m_topLeft.y; }
    constexpr T bottom() const { return m_topLeft.y + m_size.height(); }

    constexpr void setLeft(T left) { m_topLeft.x = left; }
    constexpr void setRight(T right) { m_topLeft.x = right - m_size.width(); }
    constexpr void setTop(T top) { m_topLeft.y = top; }
    constexpr void setBottom(T bottom) { m_topLeft.y = bottom - m_size.height(); }

    constexpr bool intersects(const Rect &other) const
    {
        if (right() < other.left() || left() >= other.right() || bottom() < other.top() || top() >= other.bottom())
            return false;
        return true;
    }

    constexpr bool isNull() const { return m_size.isNull(); }

    constexpr bool operator==(const Rect &other) const = default;

    constexpr friend Rect operator&(const Rect &lhs, const Rect &rhs)
    {
#if 0
        // TODO: make glm::max constexpr
        const auto topLeft = glm::max(lhs.topLeft(), rhs.topLeft());
        const auto bottomRight = glm::max(topLeft, glm::min(lhs.bottomRight(), rhs.bottomRight()));
        return Rect{topLeft, bottomRight};
#else
        const auto left = std::max(lhs.left(), rhs.left());
        const auto right = std::max(left, std::min(lhs.right(), rhs.right()));

        const auto top = std::max(lhs.top(), rhs.top());
        const auto bottom = std::max(top, std::min(lhs.bottom(), rhs.bottom()));

        return Rect{Point{left, top}, Point{right, bottom}};
#endif
    }
    constexpr Rect &operator&=(const Rect &other) { return *this = *this & other; }

    constexpr Rect intersected(const Rect &other) const { return *this & other; }

private:
    Point m_topLeft{0};
    Size m_size;
};

using RectI = Rect<int>;
using RectF = Rect<float>;

template<typename T>
struct std::formatter<Rect<T>> : std::formatter<std::string_view>
{
    constexpr auto format(const Rect<T> &rect, std::format_context &ctx) const
    {
        auto s = std::format("Rect({},{} {})", rect.left(), rect.top(), rect.size());
        return std::formatter<std::string_view>::format(s, ctx);
    }
};

// Tests
static_assert(RectI{10, 20, 30, 50}.intersects(RectI{30, 40, 80, 60}));
static_assert((RectI{10, 20, 30, 50} & RectI{30, 40, 80, 60}) == RectI{30, 40, 10, 30});
static_assert(RectI{30, 40, 80, 60}.intersects(RectI{10, 20, 30, 50}));
static_assert((RectI{30, 40, 80, 60} & RectI{10, 20, 30, 50}) == RectI{30, 40, 10, 30});
static_assert(!RectI{10, 20, 30, 50}.intersects(RectI{50, 40, 80, 60}));
static_assert((RectI{10, 20, 30, 50} & RectI{50, 40, 80, 60}).isNull());
