#pragma once

#include <ranges>
#include <concepts>

#include <muslots/muslots.h>

#include "painter.h"
#include "rect.h"
#include "window_base.h"

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

enum class HorizontalAlign
{
    Left,
    Center,
    Right
};

enum class VerticalAlign
{
    Top,
    Center,
    Bottom
};

class Gizmo
{
public:
    enum class Option
    {
        None = 0,
        FillBackground = 1 << 0,
        Hoverable = 1 << 1,
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

    void removeChild(std::size_t index);
    const Gizmo *childAt(std::size_t index) const;
    Gizmo *childAt(std::size_t index);
    std::size_t childCount() const { return m_children.size(); }

    auto children() const
    {
        return m_children | std::views::transform([](const auto &child) { return child.m_gizmo.get(); });
    }

    virtual void updateLayout();

    HorizontalAlign horizontalAlign() const { return m_horizontalAlign; }
    void setHorizontalAlign(HorizontalAlign align);

    VerticalAlign verticalAlign() const { return m_verticalAlign; }
    void setVerticalAlign(VerticalAlign align);

    glm::vec2 globalPosition() const;

    virtual bool handleMousePress(const glm::vec2 &pos);

    // these will only be called if returned true to corresponding handleMousePress
    virtual void handleMouseRelease(const glm::vec2 &pos);
    virtual void handleMouseMove(const glm::vec2 &pos);

    virtual void handleMouseEnter();
    virtual void handleMouseLeave();

    template<typename Pred>
        requires requires(const Pred &pred, Gizmo *gizmo, const glm::vec2 &pos) {
            { pred(gizmo, pos) } -> std::same_as<bool>;
        }
    Gizmo *findChildAt(const glm::vec2 &pos, const Pred &pred)
    {
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

    muslots::Signal<SizeF> resizedSignal;
    muslots::Signal<HorizontalAlign> horizontalAlignChangedSignal;
    muslots::Signal<VerticalAlign> verticalAlignChangedSignal;

    glm::vec4 backgroundColor;

protected:
    virtual void paintContents(Painter *painter, const glm::vec2 &pos, int depth) const;
    void setSize(const SizeF &size);
    glm::vec2 childOffset(const Gizmo *gizmo) const;
    void paintBackground(Painter *painter, const glm::vec2 &pos, int depth) const;
    void paintChildren(Painter *painter, const glm::vec2 &pos, int depth) const;

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
        muslots::Connection m_horizontalAlignChangedConnection;
        muslots::Connection m_verticalAlignChangedConnection;
    };

    Option m_options{Option::FillBackground};
    Gizmo *m_parent{nullptr};
    SizeF m_size;
    std::vector<ChildGizmo> m_children;
    HorizontalAlign m_horizontalAlign{HorizontalAlign::Left}; // only used if in a Column
    VerticalAlign m_verticalAlign{VerticalAlign::Top};        // only used if in a Row
};

class Rectangle : public Gizmo
{
public:
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
    explicit ScrollArea(float width, float height, Gizmo *parent = nullptr);
    explicit ScrollArea(const SizeF &size, Gizmo *parent = nullptr);

    void setSize(float width, float height);
    using Gizmo::setSize;

    bool handleMousePress(const glm::vec2 &pos) override;
    void handleMouseRelease(const glm::vec2 &pos) override;
    void handleMouseMove(const glm::vec2 &pos) override;

    void updateLayout() override;

    void setOffset(const glm::vec2 &offset);

    void paintContents(Painter *painter, const glm::vec2 &pos, int depth) const override;

private:
    bool m_dragging{false};
    glm::vec2 m_lastMousePos;
    glm::vec2 m_offset{0.0f};
    SizeF m_contentsSize;
};

class EventManager
{
public:
    EventManager();

    void setRoot(Gizmo *root);

    void handleMouseButton(MouseButton button, MouseAction action, const glm::vec2 &pos, Modifier mods);
    void handleMouseMove(const glm::vec2 &pos);

private:
    Gizmo *m_root{nullptr};
    Gizmo *m_mouseEventTarget{nullptr};
    Gizmo *m_underCursor{nullptr};
};

} // namespace ui
