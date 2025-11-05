#include "gui.h"

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
    return index < m_children.size() ? m_children[index].get() : nullptr;
}

Gizmo *Gizmo::childAt(std::size_t index)
{
    return const_cast<Gizmo *>(const_cast<const Gizmo *>(this)->childAt(index));
}

void Gizmo::updateLayout() {}

void Gizmo::paint(Painter *painter, const glm::vec2 &position, int depth) const
{
    const auto size = this->size();
    const std::array<glm::vec2, 4> verts = {position, position + glm::vec2(size.width(), 0),
                                            position + glm::vec2(size.width(), size.height()),
                                            position + glm::vec2(0, size.height())};
    painter->setColor(m_backgroundColor);
    painter->drawFilledConvexPolygon(verts, depth);
}

Rectangle::Rectangle(const SizeF &size)
    : m_size(size)
{
}

void Rectangle::setSize(const SizeF &size)
{
    m_size = size;
}

void Layout::setSpacing(float spacing)
{
    m_spacing = spacing;
}

void Row::updateLayout()
{
    float width = 0.0f;
    float height = 0.0f;
    for (const auto &child : m_children)
    {
        const auto size = child->size();
        width += size.width();
        height = std::max(height, size.height());
    }
    if (const size_t childCount = m_children.size())
        width += childCount * m_spacing;
    m_size = SizeF{width, height};
}

void Column::updateLayout()
{
    float width = 0.0f;
    float height = 0.0f;
    for (const auto &child : m_children)
    {
        const auto size = child->size();
        width = std::max(width, size.width());
        height += size.height();
    }
    if (const size_t childCount = m_children.size())
        width += childCount * m_spacing;
    m_size = SizeF{width, height};
}
