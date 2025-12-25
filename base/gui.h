#pragma once

#include "painter.h"
#include "rect.h"
#include "window_base.h"

#include <muslots/muslots.h>

#include <ranges>
#include <concepts>

namespace ui
{

struct Margins
{
    float left{0.0f};
    float right{0.0f};
    float top{0.0f};
    float bottom{0.0f};

    bool operator==(const Margins &) const = default;
};

enum class Align
{
    Left = 1 << 0,
    HorizontalCenter = 1 << 1,
    Right = 1 << 2,
    Top = 1 << 3,
    VerticalCenter = 1 << 4,
    Bottom = 1 << 5
};

constexpr Align operator&(Align x, Align y)
{
    return static_cast<Align>(static_cast<unsigned>(x) & static_cast<unsigned>(y));
}

constexpr Align &operator&=(Align &x, Align y)
{
    return x = x & y;
}

constexpr Align operator|(Align x, Align y)
{
    return static_cast<Align>(static_cast<unsigned>(x) | static_cast<unsigned>(y));
}

constexpr Align &operator|=(Align &x, Align y)
{
    return x = x | y;
}

struct Length
{
    static constexpr Length pixels(float value) { return {Type::Pixels, value}; }
    static constexpr Length percent(float value) { return {Type::Percent, value}; }

    enum class Type
    {
        Pixels,
        Percent,
    };
    Type type{Type::Pixels};
    float value{0.0f};

    constexpr bool operator==(const Length &other) const = default;
};

struct HorizontalAnchor
{
    enum class Type
    {
        Left,
        Center,
        Right
    };
    Type type{Type::Left};
    Length position;

    bool operator==(const HorizontalAnchor &) const = default;
};

struct VerticalAnchor
{
    enum class Type
    {
        Top,
        Center,
        Bottom
    };
    Type type{Type::Top};
    Length position;

    bool operator==(const VerticalAnchor &) const = default;
};

inline namespace literals
{

constexpr Length operator""_px(long double value)
{
    return Length::pixels(value);
}

constexpr Length operator""_pct(long double value)
{
    return Length::percent(value);
}

} // namespace literals

class Gizmo
{
public:
    enum class Option
    {
        None = 0,
        FillBackground = 1 << 0,
        Hoverable = 1 << 1,
        MouseTracking = 1 << 2,
    };

    friend constexpr Option operator&(Option x, Option y)
    {
        return static_cast<Option>(static_cast<unsigned>(x) & static_cast<unsigned>(y));
    }
    friend constexpr Option &operator&=(Option &x, Option y) { return x = x & y; }

    friend constexpr Option operator|(Option x, Option y)
    {
        return static_cast<Option>(static_cast<unsigned>(x) | static_cast<unsigned>(y));
    }
    friend constexpr Option &operator|=(Option &x, Option y) { return x = x | y; }

    friend constexpr Option operator~(Option x) { return static_cast<Option>(~static_cast<unsigned>(x)); }

    explicit Gizmo(Gizmo *parent = nullptr);
    virtual ~Gizmo();

    Option options() const { return m_options; }
    void setOptions(Option options);

    SizeF size() const { return m_size; }
    float width() const { return m_size.width(); }
    float height() const { return m_size.height(); }
    RectF rect() const { return RectF{glm::vec2{0.0f}, m_size}; }

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible);

    void paint(Painter *painter, const glm::vec2 &pos, int depth) const;

    template<std::derived_from<Gizmo> ChildT, typename... Args>
    ChildT *appendChild(Args &&...args)
    {
        return insertChild<ChildT>(m_children.size(), std::forward<Args>(args)...);
    }

    template<std::derived_from<Gizmo> ChildT, typename... Args>
    ChildT *insertChild(std::size_t index, Args &&...args)
    {
        auto it = m_children.emplace(std::next(m_children.begin(), index),
                                     std::make_unique<ChildT>(std::forward<Args>(args)..., this), this);
        updateLayout();
        return static_cast<ChildT *>(it->m_gizmo.get());
    }

    void removeChild(const Gizmo *gizmo);
    void removeChildAt(std::size_t index);
    const Gizmo *childAt(std::size_t index) const;
    Gizmo *childAt(std::size_t index);
    std::size_t childCount() const { return m_children.size(); }
    void clear();

    auto children() const
    {
        return m_children | std::views::transform([](const auto &child) { return child.m_gizmo.get(); });
    }

    // called when:
    //   * children added/removed
    //   * child size changes
    //   * child layout alignment changes
    virtual void updateLayout();

    void setAlign(Align align);
    void setLeft(const Length &position);
    void setHorizontalCenter(const Length &position);
    void setRight(const Length &position);
    void setHorizontalAnchor(const HorizontalAnchor &anchor);
    HorizontalAnchor horizontalAnchor() const { return m_horizontalAnchor; }

    void setTop(const Length &position);
    void setVerticalCenter(const Length &position);
    void setBottom(const Length &position);
    void setVerticalAnchor(const VerticalAnchor &anchor);
    VerticalAnchor verticalAnchor() const { return m_verticalAnchor; }

    glm::vec2 globalPosition() const;

    virtual bool handleMousePress(const glm::vec2 &pos);

    // these will only be called if returned true to corresponding handleMousePress
    virtual void handleMouseRelease(const glm::vec2 &pos);
    virtual void handleMouseMove(const glm::vec2 &pos);

    virtual bool handleMouseWheel(const glm::vec2 &offset);

    virtual void handleHoverEnter();
    virtual void handleHoverLeave();

    template<typename Pred>
        requires requires(const Pred &pred, Gizmo *gizmo, const glm::vec2 &pos) {
            { pred(gizmo, pos) } -> std::same_as<bool>;
        }
    Gizmo *findChildAt(const glm::vec2 &pos, const Pred &pred)
    {
        if (!m_visible)
            return nullptr;
        if (pos.x < 0.0f || pos.x >= m_size.width() || pos.y < 0.0f || pos.y >= m_size.height())
            return nullptr;
        // try children
        for (const auto &item : m_children)
        {
            if (auto *target = item.m_gizmo->findChildAt(pos - item.m_offset, pred))
                return target;
        }
        // try this
        if (pred(this, pos))
            return this;
        return nullptr;
    }

    bool fillBackground() const { return (m_options & Option::FillBackground) != Option::None; }
    void setFillBackground(bool fillBackground);

    bool hoverable() const { return (m_options & Option::Hoverable) != Option::None; }
    void setHoverable(bool hoverable);

    bool hasMouseTracking() const { return (m_options & Option::MouseTracking) != Option::None; }
    void setMouseTracking(bool mouseTracking);

    muslots::Signal<> aboutToBeDestroyedSignal;
    muslots::Signal<SizeF> resizedSignal;
    muslots::Signal<bool> visibleChangedSignal;
    muslots::Signal<> anchorChangedSignal;

    glm::vec4 backgroundColor = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f};

protected:
    struct ChildGizmo
    {
        explicit ChildGizmo(std::unique_ptr<Gizmo> child, Gizmo *parent);
        ~ChildGizmo();

        ChildGizmo(ChildGizmo &) = delete;
        ChildGizmo &operator=(ChildGizmo &) = delete;

        ChildGizmo(ChildGizmo &&other) = default;
        ChildGizmo &operator=(ChildGizmo &&other) = default;

        std::unique_ptr<Gizmo> m_gizmo;
        glm::vec2 m_offset{0.0f};
        muslots::Connection m_resizedConnection;
        muslots::Connection m_visibleChangedConnection;
        muslots::Connection m_anchorChangedConnection;
    };

    virtual void paintContents(Painter *painter, const glm::vec2 &pos, int depth) const;
    virtual void paintChildren(Painter *painter, const glm::vec2 &pos, int depth) const;
    void setSize(const SizeF &size);
    glm::vec2 childOffset(const Gizmo *gizmo) const;

    Option m_options{Option::None};
    Gizmo *m_parent{nullptr};
    SizeF m_size;
    bool m_visible{true};
    std::vector<ChildGizmo> m_children;
    HorizontalAnchor m_horizontalAnchor;
    VerticalAnchor m_verticalAnchor;
};

class Rectangle : public Gizmo
{
public:
    using Gizmo::Gizmo;
    explicit Rectangle(float width, float height, Gizmo *parent = nullptr);
    explicit Rectangle(const SizeF &size, Gizmo *parent = nullptr);

    void setSize(float width, float height);
    using Gizmo::setSize;
};

class Layout : public Gizmo
{
public:
    using Gizmo::Gizmo;

    float spacing() const { return m_spacing; }
    void setSpacing(float spacing);

    Margins margins() const { return m_margins; }
    void setMargins(const Margins &margins);
    void setMargins(float left, float right, float top, float bottom);
    void setMargins(float margins);

protected:
    float m_spacing{4.0f};
    Margins m_margins;
};

class Row : public Layout
{
public:
    using Layout::Layout;

    void updateLayout() override;

    void setMinimumHeight(float height);

private:
    float m_minimumHeight{0.0f};
};

class Column : public Layout
{
public:
    using Layout::Layout;

    void updateLayout() override;

    void setMinimumWidth(float width);

private:
    float m_minimumWidth{0.0f};
};

class ScrollArea : public Gizmo
{
public:
    explicit ScrollArea(Gizmo *parent = nullptr);
    explicit ScrollArea(float width, float height, Gizmo *parent = nullptr);
    explicit ScrollArea(const SizeF &size, Gizmo *parent = nullptr);

    void setSize(float width, float height);
    using Gizmo::setSize;

    void setOffset(const glm::vec2 &offset);

    void setVerticalScrollbarWidth(float width);
    float verticalScrollbarWidth() const { return m_verticalScrollbarWidth; }

    void setHorizontalScrollbarHeight(float height);
    float horizontalScrollbarHeight() const { return m_horizontalScrollbarHeight; }

    bool verticalScrollbarVisible() const { return m_verticalScrollbarVisible; }
    bool horizontalScrollbarVisible() const { return m_horizontalScrollbarVisible; }

    bool handleMousePress(const glm::vec2 &pos) override;
    void handleMouseRelease(const glm::vec2 &pos) override;
    void handleMouseMove(const glm::vec2 &pos) override;
    void handleHoverLeave() override;
    bool handleMouseWheel(const glm::vec2 &offset) override;

    void updateLayout() override;

    void paintContents(Painter *painter, const glm::vec2 &pos, int depth) const override;
    void paintChildren(Painter *painter, const glm::vec2 &pos, int depth) const override;

    glm::vec4 scrollbarColor = glm::vec4{0.5, 0.5, 0.5, 1.0f};
    glm::vec4 scrollbarHoveredColor = glm::vec4{0.75f, 0.75f, 0.75f, 1.0f};
    glm::vec4 scrollbarPressedColor = glm::vec4{1.0f};

private:
    static constexpr auto kScrollbarSpacing = 1.0f;

    enum class DragState
    {
        None,
        HorizontalScrollbar,
        VerticalScrollbar
    };

    RectF verticalScrollbarRect() const;
    RectF horizontalScrollbarRect() const;

    DragState m_dragState{DragState::None};
    glm::vec2 m_lastMousePos;
    glm::vec2 m_offset{0.0f};
    SizeF m_contentsSize;
    SizeF m_viewportSize;
    float m_verticalScrollbarWidth{12.0f};
    float m_horizontalScrollbarHeight{12.0f};
    bool m_verticalScrollbarVisible{false};
    bool m_verticalScrollbarHovered{false};
    bool m_horizontalScrollbarVisible{false};
    bool m_horizontalScrollbarHovered{false};
};

class Text : public Gizmo
{
public:
    using Gizmo::Gizmo;
    explicit Text(std::string_view text, Gizmo *parent = nullptr);
    explicit Text(const Font &font, std::string_view text, Gizmo *parent = nullptr);

    void setText(std::string_view text);
    std::string_view text() const { return m_text; }

    void setFont(const Font &font);
    Font font() const { return m_font; }

    glm::vec4 color{1.0f};

protected:
    void paintContents(Painter *painter, const glm::vec2 &pos, int depth) const override;

private:
    void updateSize();

    std::string m_text;
    Font m_font;
};

class MultiLineText : public Gizmo
{
public:
    using Gizmo::Gizmo;
    explicit MultiLineText(std::string_view text, Gizmo *parent = nullptr);
    explicit MultiLineText(const Font &font, std::string_view text, Gizmo *parent = nullptr);

    void setText(std::string_view text);
    std::string_view text() const { return m_text; }

    void setFont(const Font &font);
    Font font() const { return m_font; }

    void setLineWidth(float width);
    float lineWidth() const { return m_lineWidth; }

    glm::vec4 color{1.0f};

protected:
    void paintContents(Painter *painter, const glm::vec2 &pos, int depth) const override;

private:
    void updateTextLayout();

    std::string m_text;
    Font m_font;
    std::vector<std::string_view> m_lines;
    float m_lineWidth{0.0f};
};

class Icon : public Gizmo
{
public:
    using Gizmo::Gizmo;
    explicit Icon(std::string_view source, Gizmo *parent = nullptr);

    void setSource(std::string_view source);
    std::string_view source() const { return m_source; }

    glm::vec4 color{1.0f};

protected:
    void paintContents(Painter *painter, const glm::vec2 &pos, int depth) const override;

private:
    std::string m_source;
};

template<std::derived_from<Gizmo> GizmoT>
class GizmoWeakPtr
{
public:
    GizmoWeakPtr(GizmoT *gizmo = nullptr) { reset(gizmo); }
    ~GizmoWeakPtr() { m_destroyedConnection.disconnect(); }

    // TODO implement these
    GizmoWeakPtr(GizmoWeakPtr &&) = delete;
    GizmoWeakPtr &operator=(GizmoWeakPtr &&) = delete;

    // TODO and these
    GizmoWeakPtr(const GizmoWeakPtr &) = delete;
    GizmoWeakPtr &operator=(const GizmoWeakPtr &) = delete;

    void reset(GizmoT *gizmo)
    {
        if (gizmo == m_gizmo)
            return;
        if (m_gizmo)
            m_destroyedConnection.disconnect();
        m_gizmo = gizmo;
        if (m_gizmo)
            m_destroyedConnection = m_gizmo->aboutToBeDestroyedSignal.connect([this] { reset(nullptr); });
    }

    GizmoT *get() const { return m_gizmo; }
    GizmoT *operator->() const { return get(); }
    explicit operator bool() const { return m_gizmo != nullptr; }

private:
    GizmoT *m_gizmo;
    muslots::Connection m_destroyedConnection;
};

class EventManager
{
public:
    EventManager();

    void setRoot(Gizmo *root);

    bool handleMouseButton(MouseButton button, MouseAction action, const glm::vec2 &pos, Modifier mods);
    bool handleMouseMove(const glm::vec2 &pos);
    bool handleMouseWheel(const glm::vec2 &mousePos, const glm::vec2 &wheelOffset);

private:
    Gizmo *m_root{nullptr};
    GizmoWeakPtr<Gizmo> m_mouseEventTarget;
    GizmoWeakPtr<Gizmo> m_underCursor;
};

} // namespace ui
