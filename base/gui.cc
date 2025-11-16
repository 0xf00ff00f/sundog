#include "gui.h"

namespace ui
{

Gizmo::ChildGizmo::ChildGizmo(std::unique_ptr<Gizmo> gizmo, Gizmo *parent)
    : m_gizmo(std::move(gizmo))
    , m_resizedConnection(m_gizmo->resizedSignal.connect([parent](SizeF) { parent->updateLayout(); }))
    , m_horizontalAlignChangedConnection(
          m_gizmo->horizontalAlignChangedSignal.connect([parent](HorizontalAlign) { parent->updateLayout(); }))
    , m_verticalAlignChangedConnection(
          m_gizmo->verticalAlignChangedSignal.connect([parent](VerticalAlign) { parent->updateLayout(); }))
{
}

Gizmo::ChildGizmo::~ChildGizmo()
{
    m_resizedConnection.disconnect();
}

Gizmo::Gizmo() = default;

Gizmo::~Gizmo() = default;

void Gizmo::removeChild(std::size_t index)
{
    if (index >= m_children.size())
        return;
    m_children.erase(std::next(m_children.begin(), index));
    updateLayout();
}

const Gizmo *Gizmo::childAt(std::size_t index) const
{
    return index < m_children.size() ? m_children[index].m_gizmo.get() : nullptr;
}

Gizmo *Gizmo::childAt(std::size_t index)
{
    return const_cast<Gizmo *>(const_cast<const Gizmo *>(this)->childAt(index));
}

void Gizmo::updateLayout() {}

void Gizmo::paint(Painter *painter, const glm::vec2 &position, int depth) const
{
    if (fillBackground)
    {
        const auto size = this->size();
        const std::array<glm::vec2, 4> verts = {position, position + glm::vec2(size.width(), 0),
                                                position + glm::vec2(size.width(), size.height()),
                                                position + glm::vec2(0, size.height())};
        painter->setColor(backgroundColor);
        painter->drawFilledConvexPolygon(verts, depth);
    }

    for (const auto &item : m_children)
    {
        const auto *child = item.m_gizmo.get();
        child->paint(painter, position + item.m_offset, depth + 1);
    }
}

void Gizmo::setSize(const SizeF &size)
{
    if (size == m_size)
        return;
    m_size = size;
    resizedSignal(m_size);
}

void Gizmo::setHorizontalAlign(HorizontalAlign align)
{
    if (align == m_horizontalAlign)
        return;
    m_horizontalAlign = align;
    horizontalAlignChangedSignal(m_horizontalAlign);
}

void Gizmo::setVerticalAlign(VerticalAlign align)
{
    if (align == m_verticalAlign)
        return;
    m_verticalAlign = align;
    verticalAlignChangedSignal(m_verticalAlign);
}

Rectangle::Rectangle(const SizeF &size)
{
    m_size = size;
}

Rectangle::Rectangle(float width, float height)
    : Rectangle(SizeF{width, height})
{
}

void Rectangle::setSize(float width, float height)
{
    setSize(SizeF{width, height});
}

void Layout::setSpacing(float spacing)
{
    if (spacing == m_spacing)
        return;
    m_spacing = spacing;
    updateLayout();
}

void Layout::setMargins(const Margins &margins)
{
    if (margins == m_margins)
        return;
    m_margins = margins;
    updateLayout();
}

void Row::setMinimumHeight(float height)
{
    if (height == m_minimumHeight)
        return;
    m_minimumHeight = height;
    updateLayout();
}

void Row::updateLayout()
{
    // update size
    float width = 0.0f;
    float height = 0.0f;
    for (const auto *child : children())
    {
        const auto size = child->size();
        width += size.width();
        height = std::max(height, size.height());
    }
    if (const size_t childCount = m_children.size())
        width += (childCount - 1) * m_spacing;
    width += m_margins.left + m_margins.right;
    height += m_margins.top + m_margins.bottom;
    height = std::max(m_minimumHeight, height);
    setSize(SizeF{width, height});

    // update child offsets
    float x = m_margins.left;
    for (auto &item : m_children)
    {
        const auto *child = item.m_gizmo.get();
        const auto childSize = child->size();
        const auto y = [this, child, &childSize] {
            switch (child->verticalAlign())
            {
            case VerticalAlign::Top:
            default: {
                return m_margins.top;
            }
            case VerticalAlign::Center: {
                const auto usableHeight = m_size.height() - (m_margins.top + m_margins.bottom);
                return m_margins.top + 0.5f * (usableHeight - childSize.height());
            }
            case VerticalAlign::Bottom: {
                return m_size.height() - m_margins.bottom - childSize.height();
            }
            }
        }();
        item.m_offset = glm::vec2{x, y};
        x += childSize.width() + m_spacing;
    }
}

void Column::setMinimumWidth(float width)
{
    if (width == m_minimumWidth)
        return;
    m_minimumWidth = width;
    updateLayout();
}

void Column::updateLayout()
{
    // update size
    float width = 0.0f;
    float height = 0.0f;
    for (const auto *child : children())
    {
        const auto size = child->size();
        width = std::max(width, size.width());
        height += size.height();
    }
    if (const size_t childCount = m_children.size())
        height += (childCount - 1) * m_spacing;
    width += m_margins.left + m_margins.right;
    width = std::max(m_minimumWidth, width);
    height += m_margins.top + m_margins.bottom;
    setSize(SizeF{width, height});

    // update child offsets
    float y = m_margins.top;
    for (auto &item : m_children)
    {
        const auto *child = item.m_gizmo.get();
        const auto childSize = child->size();
        const auto x = [this, child, &childSize] {
            switch (child->horizontalAlign())
            {
            case HorizontalAlign::Left:
            default: {
                return m_margins.left;
            }
            case HorizontalAlign::Center: {
                const auto usableWidth = m_size.width() - (m_margins.left + m_margins.right);
                return m_margins.left + 0.5f * (usableWidth - childSize.width());
            }
            case HorizontalAlign::Right: {
                return m_size.width() - m_margins.right - childSize.width();
            }
            }
        }();
        item.m_offset = glm::vec2{x, y};
        y += childSize.height() + m_spacing;
    }
}

} // namespace ui
