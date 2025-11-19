#include "gui.h"

#include <cassert>
#include <print>

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
    m_horizontalAlignChangedConnection.disconnect();
    m_verticalAlignChangedConnection.disconnect();
}

Gizmo::Gizmo(Gizmo *parent)
    : m_parent(parent)
{
}

Gizmo::~Gizmo() = default;

void Gizmo::setOptions(Option options)
{
    m_options = options;
}

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

void Gizmo::paint(Painter *painter, const glm::vec2 &pos, int depth) const
{
    auto clipRect = painter->clipRect();
    if (clipRect.isNull())
        return;
    auto rect = RectF{pos, m_size};
    if (!clipRect.intersects(rect))
        return;
    paintContents(painter, pos, depth);
    paintChildren(painter, pos, depth);
}

void Gizmo::paintContents(Painter *painter, const glm::vec2 &pos, int depth) const
{
    if (fillBackground())
    {
        const auto size = this->size();
        const std::array<glm::vec2, 4> verts = {pos, pos + glm::vec2(size.width(), 0),
                                                pos + glm::vec2(size.width(), size.height()),
                                                pos + glm::vec2(0, size.height())};
        painter->setColor(backgroundColor);
        painter->drawFilledConvexPolygon(verts, depth);
    }
}

void Gizmo::paintChildren(Painter *painter, const glm::vec2 &pos, int depth) const
{
    for (const auto &item : m_children)
    {
        const auto *child = item.m_gizmo.get();
        child->paint(painter, pos + item.m_offset, depth + 1);
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

glm::vec2 Gizmo::globalPosition() const
{
    glm::vec2 position{0.0f};
    auto *gizmo = this;
    while (gizmo->m_parent)
    {
        position += gizmo->m_parent->childOffset(gizmo);
        gizmo = gizmo->m_parent;
    }
    return position;
}

glm::vec2 Gizmo::childOffset(const Gizmo *gizmo) const
{
    auto it = std::ranges::find_if(m_children, [gizmo](const auto &item) { return item.m_gizmo.get() == gizmo; });
    assert(it != m_children.end());
    if (it == m_children.end())
    {
        return {};
    }
    return it->m_offset;
}

bool Gizmo::handleMousePress(const glm::vec2 &)
{
    return false;
}

void Gizmo::handleMouseRelease(const glm::vec2 &) {}

void Gizmo::handleMouseMove(const glm::vec2 &) {}

void Gizmo::handleMouseEnter() {}

void Gizmo::handleMouseLeave() {}

void Gizmo::setHoverable(bool hoverable)
{
    m_options &= ~Option::Hoverable;
    if (hoverable)
        m_options |= Option::Hoverable;
}

void Gizmo::setFillBackground(bool fillBackground)
{
    m_options &= ~Option::FillBackground;
    if (fillBackground)
        m_options |= Option::FillBackground;
}

Rectangle::Rectangle(const SizeF &size, Gizmo *parent)
    : Gizmo(parent)
{
    m_size = size;
}

Rectangle::Rectangle(float width, float height, Gizmo *parent)
    : Rectangle(SizeF{width, height}, parent)
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

void Layout::setMargins(float left, float right, float top, float bottom)
{
    setMargins(Margins{.left = left, .right = right, .top = top, .bottom = bottom});
}

void Layout::setMargins(float margins)
{
    setMargins(Margins{.left = margins, .right = margins, .top = margins, .bottom = margins});
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

ScrollArea::ScrollArea(float width, float height, Gizmo *parent)
    : ScrollArea(SizeF{width, height}, parent)
{
}

ScrollArea::ScrollArea(const SizeF &size, Gizmo *parent)
    : Gizmo(parent)
{
    m_size = size;
}

void ScrollArea::setSize(float width, float height)
{
    setSize(SizeF{width, height});
}

bool ScrollArea::handleMousePress(const glm::vec2 &pos)
{
    m_dragging = true;
    m_lastMousePos = pos;
    return true;
}

void ScrollArea::handleMouseRelease(const glm::vec2 &pos)
{
    m_dragging = false;
}

void ScrollArea::handleMouseMove(const glm::vec2 &pos)
{
    if (m_dragging)
    {
        const auto delta = pos - m_lastMousePos;
        setOffset(m_offset + delta);
        m_lastMousePos = pos;
    }
}

void ScrollArea::setOffset(const glm::vec2 &offset)
{
    const auto maxOffset =
        glm::vec2{m_size.width() - m_contentsSize.width(), m_size.height() - m_contentsSize.height()};
    const auto clampedOffset = glm::min(glm::vec2{0.0f, 0.0f}, glm::max(maxOffset, offset));
    if (clampedOffset == m_offset)
        return;

    m_offset = clampedOffset;

    for (auto &item : m_children)
        item.m_offset = m_offset;
}

void ScrollArea::updateLayout()
{
    float contentsWidth = 0.0f;
    float contentsHeight = 0.0f;
    for (const auto *child : children())
    {
        const auto childSize = child->size();
        contentsWidth = std::max(contentsWidth, childSize.width());
        contentsHeight = std::max(contentsHeight, childSize.height());
    }
    m_contentsSize = SizeF{contentsWidth, contentsHeight};

    setOffset(m_offset);
}

void ScrollArea::paintChildren(Painter *painter, const glm::vec2 &pos, int depth) const
{
    const RectF prevClipRect = painter->clipRect();
    const auto clipRect = RectF{pos, m_size};
    painter->setClipRect(clipRect & painter->clipRect());
    Gizmo::paintChildren(painter, pos, depth);
    painter->setClipRect(prevClipRect);
}

Text::Text(std::string_view text, Gizmo *parent)
    : Text({}, text, parent)
{
}

Text::Text(const Font &font, std::string_view text, Gizmo *parent)
    : Gizmo(parent)
    , m_font(font)
    , m_text(text)
{
    updateSize();
}

void Text::setText(std::string_view text)
{
    if (text == m_text)
        return;
    m_text = text;
    updateSize();
}

void Text::setFont(const Font &font)
{
    if (font == m_font)
        return;
    m_font = font;
    updateSize();
}

void Text::updateSize()
{
    if (m_font.isNull() || m_text.empty())
        setSize({});
    const FontMetrics fm{m_font};
    setSize(SizeF{fm.horizontalAdvance(m_text), fm.pixelHeight()});
}

void Text::paintContents(Painter *painter, const glm::vec2 &pos, int depth) const
{
    Gizmo::paintContents(painter, pos, depth);
    painter->setColor(color);
    painter->setFont(m_font);
    painter->drawText(pos, m_text, depth);
}

MultiLineText::MultiLineText(std::string_view text, Gizmo *parent)
    : MultiLineText({}, text, parent)
{
}

MultiLineText::MultiLineText(const Font &font, std::string_view text, Gizmo *parent)
    : Gizmo(parent)
    , m_font(font)
    , m_text(text)
{
    updateTextLayout();
}

void MultiLineText::setText(std::string_view text)
{
    std::string compressedText;
    auto it = text.begin();
    // skip leading spaces
    while (it != text.end() && *it == ' ')
        ++it;
    // remove extra spaces
    while (it != text.end())
    {
        if (!std::isspace(*it))
        {
            compressedText.push_back(*it++);
        }
        else
        {
            it = std::find_if(it + 1, text.end(), [](auto c) { return !std::isspace(c); });
            if (it != text.end())
                compressedText.push_back(' ');
        }
    }
    if (compressedText == m_text)
        return;
    m_text = compressedText;
    updateTextLayout();
}

void MultiLineText::setFont(const Font &font)
{
    if (font == m_font)
        return;
    m_font = font;
    updateTextLayout();
}

void MultiLineText::setLineWidth(float width)
{
    if (width == m_lineWidth)
        return;
    m_lineWidth = width;
    updateTextLayout();
}

void MultiLineText::updateTextLayout()
{
    m_lines.clear();

    if (m_font.isNull() || m_text.empty())
        setSize({});

    const FontMetrics fm{m_font};

    auto curLineStart = m_text.begin();
    std::optional<std::string::iterator> prevWordEnd;
    for (auto it = m_text.begin(); it != m_text.end(); ++it)
    {
        if (*it == ' ')
        {
            prevWordEnd = it;
        }
        else
        {
            const auto breakLine = [&] {
                if (!prevWordEnd)
                    return false;
                // FIXME quadratic
                const auto curLineWidth = fm.horizontalAdvance(std::string_view{curLineStart, std::next(it)});
                return curLineWidth > m_lineWidth;
            }();
            if (breakLine)
            {
                assert(prevWordEnd);
                m_lines.push_back(std::string_view{curLineStart, *prevWordEnd});
                curLineStart = std::next(*prevWordEnd);
                prevWordEnd.reset();
            }
        }
    }
    if (curLineStart != m_text.end())
    {
        m_lines.push_back(std::string_view{curLineStart, m_text.end()});
    }

    setSize(SizeF{m_lineWidth, m_lines.size() * fm.pixelHeight()});
}

void MultiLineText::paintContents(Painter *painter, const glm::vec2 &pos, int depth) const
{
    Gizmo::paintContents(painter, pos, depth);
    painter->setColor(color);
    painter->setFont(m_font);

    const auto lineHeight = FontMetrics{m_font}.pixelHeight();
    auto linePos = pos;
    for (const auto &line : m_lines)
    {
        painter->drawText(linePos, line, depth);
        linePos.y += lineHeight;
    }
}

EventManager::EventManager() = default;

void EventManager::setRoot(Gizmo *root)
{
    m_root = root;
}

void EventManager::handleMouseButton(MouseButton button, MouseAction action, const glm::vec2 &pos, Modifier mods)
{
    if (button != MouseButton::Left)
        return;
    switch (action)
    {
    case MouseAction::Press: {
        auto *target =
            m_root->findChildAt(pos, [](Gizmo *gizmo, const glm::vec2 &pos) { return gizmo->handleMousePress(pos); });
        if (target)
        {
            // found a gizmo that accepts the mouse press, will get mouse move and the mouse release event
            m_mouseEventTarget = target;
        }
        break;
    }
    case MouseAction::Release: {
        if (m_mouseEventTarget)
        {
            m_mouseEventTarget->handleMouseRelease(pos - m_mouseEventTarget->globalPosition());
            m_mouseEventTarget = nullptr;
        }
        break;
    }
    }
}

void EventManager::handleMouseMove(const glm::vec2 &pos)
{
    auto *underCursor = m_root->findChildAt(pos, [](Gizmo *gizmo, const glm::vec2 &) { return gizmo->hoverable(); });
    if (m_underCursor != underCursor)
    {
        if (m_underCursor)
            m_underCursor->handleMouseLeave();
        m_underCursor = underCursor;
        if (m_underCursor)
            m_underCursor->handleMouseEnter();
    }

    if (m_mouseEventTarget)
    {
        m_mouseEventTarget->handleMouseMove(pos - m_mouseEventTarget->globalPosition());
    }
}

} // namespace ui
