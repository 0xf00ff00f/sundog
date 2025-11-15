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
                                     std::make_unique<ChildT>(std::forward<Args>(args)...), this);
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

    muslots::Signal<SizeF> resizedSignal;

    bool fillBackground{true};
    glm::vec4 backgroundColor;

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
        muslots::Connection m_resizedConnection;
    };

    std::vector<ChildGizmo> m_children;
};

class Rectangle : public Gizmo
{
public:
    explicit Rectangle(float width, float height);
    explicit Rectangle(const SizeF &size);

    SizeF size() const override { return m_size; }

    void setSize(float width, float height);
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
    void paint(Painter *painter, const glm::vec2 &position, int depth) const override;
};

class Column : public Layout
{
public:
    using Layout::Layout;

    void updateLayout() override;
    void paint(Painter *painter, const glm::vec2 &position, int depth) const override;
};
