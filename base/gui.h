#pragma once

#include <ranges>
#include <concepts>

#include <muslots/muslots.h>

#include "painter.h"
#include "rect.h"

class Gizmo
{
public:
    Gizmo();
    virtual ~Gizmo();

    virtual SizeF size() const = 0;
    virtual void paint(Painter *painter, const glm::vec2 &position, int depth) const;

    template<std::derived_from<Gizmo> ChildT, typename... Args>
    ChildT *appendChild(Args &&...args)
    {
        return insertChild<ChildT>(m_children.size(), std::forward<Args>(args)...);
    }

    template<std::derived_from<Gizmo> ChildT, typename... Args>
    ChildT *insertChild(std::size_t index, Args &&...args)
    {
        auto it = m_children.emplace(std::next(m_children.begin(), index),
                                     std::make_unique<ChildT>(std::forward<Args>(args)...));
        updateLayout();
        return static_cast<ChildT *>(it->get());
    }

    void removeChild(std::size_t index);
    const Gizmo *childAt(std::size_t index) const;
    Gizmo *childAt(std::size_t index);
    std::size_t childCount() const { return m_children.size(); }

    auto children() const
    {
        return m_children | std::views::transform([](const auto &child) { return child.get(); });
    }

    virtual void updateLayout();

    muslots::Signal<SizeF> resizedSignal;

protected:
    std::vector<std::unique_ptr<Gizmo>> m_children;
    bool m_fillBackground{true};
    glm::vec4 m_backgroundColor;
};

class Rectangle : public Gizmo
{
public:
    explicit Rectangle(const SizeF &size);

    SizeF size() const override { return m_size; }

    void setSize(const SizeF &size);

protected:
    SizeF m_size;
};

class Layout : public Gizmo
{
public:
    using Gizmo::Gizmo;

    SizeF size() const override { return m_size; }

    float spacing() const { return m_spacing; }
    void setSpacing(float spacing);

protected:
    float m_spacing{4.0f};

    SizeF m_size;
};

class Row : public Layout
{
public:
    using Layout::Layout;

    void updateLayout() override;
};

class Column : public Layout
{
public:
    using Layout::Layout;

    void updateLayout() override;
};
