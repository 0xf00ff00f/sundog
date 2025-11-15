#include "gui.h"

Gizmo::ChildGizmo::ChildGizmo(std::unique_ptr<Gizmo> gizmo, Gizmo *parent)
    : m_gizmo(std::move(gizmo))
    , m_resizedConnection(m_gizmo->resizedSignal.connect([parent](SizeF) { parent->updateLayout(); }))
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
}

void Gizmo::setSize(const SizeF &size)
{
    if (size == m_size)
        return;
    m_size = size;
    resizedSignal(m_size);
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
}

void Row::paint(Painter *painter, const glm::vec2 &position, int depth) const
{
    Gizmo::paint(painter, position, depth);
    auto p = position + glm::vec2{m_margins.left, m_margins.top};
    for (const auto *child : children())
    {
        // TODO alignment
        const auto childSize = child->size();
        child->paint(painter, p, depth + 1);
        p.x += childSize.width() + m_spacing;
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
}

void Column::paint(Painter *painter, const glm::vec2 &position, int depth) const
{
    Gizmo::paint(painter, position, depth);
    auto p = position + glm::vec2{m_margins.left, m_margins.top};
    for (const auto *child : children())
    {
        // TODO alignment
        const auto childSize = child->size();
        child->paint(painter, p, depth + 1);
        p.y += childSize.height() + m_spacing;
    }
}
