#include "gui.h"

#include "image.h"

#include <glm/gtx/string_cast.hpp>

#include <cassert>
#include <print>

namespace ui
{

Gizmo::ChildGizmo::ChildGizmo(std::unique_ptr<Gizmo> gizmo, Gizmo *parent)
    : m_gizmo(std::move(gizmo))
    , m_resizedConnection(m_gizmo->resizedSignal.connect([parent](SizeF) { parent->updateLayout(); }))
    , m_anchorChangedConnection(m_gizmo->anchorChangedSignal.connect([parent]() { parent->updateLayout(); }))
{
}

Gizmo::ChildGizmo::~ChildGizmo()
{
    m_resizedConnection.disconnect();
    m_anchorChangedConnection.disconnect();
}

Gizmo::Gizmo(Gizmo *parent)
    : m_parent(parent)
{
}

Gizmo::~Gizmo()
{
    aboutToBeDestroyedSignal();
}

void Gizmo::setOptions(Option options)
{
    m_options = options;
}

void Gizmo::clear()
{
    if (m_children.empty())
        return;
    m_children.clear();
    updateLayout();
}

void Gizmo::removeChildAt(std::size_t index)
{
    if (index >= m_children.size())
        return;
    m_children.erase(std::next(m_children.begin(), index));
    updateLayout();
}

void Gizmo::removeChild(const Gizmo *gizmo)
{
    auto it = std::ranges::find_if(m_children, [gizmo](const auto &child) { return child.m_gizmo.get() == gizmo; });
    if (it == m_children.end())
        return;
    m_children.erase(it);
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

void Gizmo::updateLayout()
{
    for (auto &item : m_children)
    {
        const auto *child = item.m_gizmo.get();
        const auto childSize = child->size();
        const auto x = [this, child, &childSize] {
            const auto anchor = child->horizontalAnchor();
            const auto anchorX = [this, position = anchor.position]() -> float {
                switch (position.type)
                {
                case Length::Type::Pixels:
                default:
                    return position.value;
                case Length::Type::Percent:
                    return (position.value / 100.0f) * m_size.width();
                }
            }();
            switch (anchor.type)
            {
            case HorizontalAnchor::Type::Left:
            default:
                return anchorX;
            case HorizontalAnchor::Type::Center:
                return anchorX - 0.5f * childSize.width();
            case HorizontalAnchor::Type::Right:
                return anchorX - childSize.width();
            }
        }();
        const auto y = [this, child, &childSize] {
            const auto anchor = child->verticalAnchor();
            const auto anchorY = [this, position = anchor.position]() -> float {
                switch (position.type)
                {
                case Length::Type::Pixels:
                default:
                    return position.value;
                case Length::Type::Percent:
                    return (position.value / 100.0f) * m_size.height();
                }
            }();
            switch (anchor.type)
            {
            case VerticalAnchor::Type::Top:
            default:
                return anchorY;
            case VerticalAnchor::Type::Center:
                return anchorY - 0.5f * childSize.height();
            case VerticalAnchor::Type::Bottom:
                return anchorY - childSize.height();
            }
            return 0.0f;
        }();
        item.m_offset = glm::vec2{x, y};
    }
}

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
        painter->setColor(backgroundColor);
        painter->fillRect(RectF{pos, m_size}, depth);
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
    updateLayout();
}

void Gizmo::setAlign(Align align)
{
    // horizontal
    switch (align & (Align::Left | Align::HorizontalCenter | Align::Right))
    {
    case Align::Left:
    default:
        setHorizontalAnchor({HorizontalAnchor::Type::Left, 0.0_px});
        break;
    case Align::HorizontalCenter:
        setHorizontalAnchor({HorizontalAnchor::Type::Center, 50.0_pct});
        break;
    case Align::Right:
        setHorizontalAnchor({HorizontalAnchor::Type::Right, 100.0_pct});
        break;
    }

    // vertical
    switch (align & (Align::Top | Align::VerticalCenter | Align::Bottom))
    {
    case Align::Top:
    default:
        setVerticalAnchor({VerticalAnchor::Type::Top, 0.0_px});
        break;
    case Align::VerticalCenter:
        setVerticalAnchor({VerticalAnchor::Type::Center, 50.0_pct});
        break;
    case Align::Bottom:
        setVerticalAnchor({VerticalAnchor::Type::Bottom, 100.0_pct});
        break;
    }
}

void Gizmo::setLeft(const Length &pos)
{
    setHorizontalAnchor({HorizontalAnchor::Type::Left, pos});
}

void Gizmo::setHorizontalCenter(const Length &pos)
{
    setHorizontalAnchor({HorizontalAnchor::Type::Center, pos});
}

void Gizmo::setRight(const Length &pos)
{
    setHorizontalAnchor({HorizontalAnchor::Type::Right, pos});
}

void Gizmo::setTop(const Length &pos)
{
    setVerticalAnchor({VerticalAnchor::Type::Top, pos});
}

void Gizmo::setVerticalCenter(const Length &pos)
{
    setVerticalAnchor({VerticalAnchor::Type::Center, pos});
}

void Gizmo::setBottom(const Length &pos)
{
    setVerticalAnchor({VerticalAnchor::Type::Bottom, pos});
}

void Gizmo::setHorizontalAnchor(const HorizontalAnchor &anchor)
{
    if (anchor == m_horizontalAnchor)
        return;
    m_horizontalAnchor = anchor;
    anchorChangedSignal();
}

void Gizmo::setVerticalAnchor(const VerticalAnchor &anchor)
{
    if (anchor == m_verticalAnchor)
        return;
    m_verticalAnchor = anchor;
    anchorChangedSignal();
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

void Gizmo::handleHoverEnter() {}

void Gizmo::handleHoverLeave() {}

bool Gizmo::handleMouseWheel(const glm::vec2 &)
{
    return false;
}

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

void Gizmo::setMouseTracking(bool mouseTracking)
{
    m_options &= ~Option::MouseTracking;
    if (mouseTracking)
        m_options |= Option::MouseTracking;
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
    const auto usableHeight = m_size.height() - (m_margins.top + m_margins.bottom);
    float x = m_margins.left;
    for (auto &item : m_children)
    {
        const auto *child = item.m_gizmo.get();
        const auto childSize = child->size();
        auto y = [this, usableHeight, child, &childSize] {
            const auto anchor = child->verticalAnchor();
            const auto anchorY = [this, usableHeight, position = anchor.position]() -> float {
                switch (position.type)
                {
                case Length::Type::Pixels:
                default:
                    return position.value;
                case Length::Type::Percent:
                    return (position.value / 100.0f) * usableHeight;
                }
            }();
            switch (anchor.type)
            {
            case VerticalAnchor::Type::Top:
            default:
                return m_margins.top + anchorY;
            case VerticalAnchor::Type::Center:
                return m_margins.top + anchorY - 0.5f * childSize.height();
            case VerticalAnchor::Type::Bottom:
                return m_margins.top + anchorY - childSize.height();
            }
            return 0.0f;
        }();
        y = std::clamp(y, m_margins.top, m_margins.top + usableHeight - childSize.height());
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
    const auto usableWidth = m_size.width() - (m_margins.left + m_margins.right);
    float y = m_margins.top;
    for (auto &item : m_children)
    {
        const auto *child = item.m_gizmo.get();
        const auto childSize = child->size();
        auto x = [this, usableWidth, child, &childSize] {
            const auto anchor = child->horizontalAnchor();
            const auto anchorX = [this, usableWidth, position = anchor.position]() -> float {
                switch (position.type)
                {
                case Length::Type::Pixels:
                default:
                    return position.value;
                case Length::Type::Percent:
                    return (position.value / 100.0f) * usableWidth;
                }
            }();
            switch (anchor.type)
            {
            case HorizontalAnchor::Type::Left:
            default:
                return m_margins.left + anchorX;
            case HorizontalAnchor::Type::Center:
                return m_margins.left + anchorX - 0.5f * childSize.width();
            case HorizontalAnchor::Type::Right:
                return m_margins.left + anchorX - childSize.width();
            }
        }();
        x = std::clamp(x, m_margins.left, m_margins.left + usableWidth - childSize.width());
        item.m_offset = glm::vec2{x, y};
        y += childSize.height() + m_spacing;
    }
}

ScrollArea::ScrollArea(Gizmo *parent)
    : ScrollArea(SizeF{}, parent)
{
}

ScrollArea::ScrollArea(float width, float height, Gizmo *parent)
    : ScrollArea(SizeF{width, height}, parent)
{
}

ScrollArea::ScrollArea(const SizeF &size, Gizmo *parent)
    : Gizmo(parent)
{
    m_size = size;
    setHoverable(true);
    setMouseTracking(true);
}

void ScrollArea::setSize(float width, float height)
{
    setSize(SizeF{width, height});
}

void ScrollArea::setVerticalScrollbarWidth(float width)
{
    if (width == m_verticalScrollbarWidth)
        return;
    m_verticalScrollbarWidth = width;
    updateLayout();
}

void ScrollArea::setHorizontalScrollbarHeight(float height)
{
    if (height == m_horizontalScrollbarHeight)
        return;
    m_horizontalScrollbarHeight = height;
    updateLayout();
}

bool ScrollArea::handleMousePress(const glm::vec2 &pos)
{
    if (const auto rect = verticalScrollbarRect(); rect && rect.contains(pos))
    {
        m_dragState = DragState::VerticalScrollbar;
        m_lastMousePos = pos;
        return true;
    }

    if (const auto rect = horizontalScrollbarRect(); rect && rect.contains(pos))
    {
        m_dragState = DragState::HorizontalScrollbar;
        m_lastMousePos = pos;
        return true;
    }

    return false;
}

void ScrollArea::handleMouseRelease(const glm::vec2 &pos)
{
    m_dragState = DragState::None;
}

void ScrollArea::handleMouseMove(const glm::vec2 &pos)
{
    switch (m_dragState)
    {
    case DragState::VerticalScrollbar: {
        const auto delta = pos.y - m_lastMousePos.y;
        const auto scale = m_viewportSize.height() / m_contentsSize.height();
        setOffset(m_offset - glm::vec2{0.0f, delta / scale});
        m_lastMousePos = pos;
        break;
    }
    case DragState::HorizontalScrollbar: {
        const auto delta = pos.x - m_lastMousePos.x;
        const auto scale = m_viewportSize.width() / m_contentsSize.width();
        setOffset(m_offset - glm::vec2{delta / scale, 0.0f});
        m_lastMousePos = pos;
        break;
    }
    default:
        break;
    }

    m_verticalScrollbarHovered = verticalScrollbarRect().contains(pos);
    m_horizontalScrollbarHovered = horizontalScrollbarRect().contains(pos);
}

void ScrollArea::handleHoverLeave()
{
    m_verticalScrollbarHovered = false;
    m_horizontalScrollbarHovered = false;
}

bool ScrollArea::handleMouseWheel(const glm::vec2 &offset)
{
    const bool canScroll =
        (offset.y != 0.0f && m_verticalScrollbarVisible) || (offset.x != 0.0f && m_horizontalScrollbarVisible);
    if (!canScroll)
        return false;
    constexpr auto kScrollSpeed = 10.0f;
    setOffset(m_offset + kScrollSpeed * offset);
    return true;
}

void ScrollArea::setOffset(const glm::vec2 &offset)
{
    const auto maxOffset =
        glm::vec2{m_viewportSize.width() - m_contentsSize.width(), m_viewportSize.height() - m_contentsSize.height()};
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

    m_viewportSize = m_size;
    if (m_contentsSize.height() > m_viewportSize.height())
    {
        m_verticalScrollbarVisible = true;
        m_viewportSize.setWidth(std::max(0.0f, m_viewportSize.width() - m_verticalScrollbarWidth));
    }
    else
    {
        m_verticalScrollbarVisible = false;
    }

    if (m_contentsSize.width() > m_viewportSize.width())
    {
        m_horizontalScrollbarVisible = true;
        m_viewportSize.setHeight(std::max(0.0f, m_viewportSize.height() - m_horizontalScrollbarHeight));
    }
    else
    {
        m_horizontalScrollbarVisible = false;
    }

    setOffset(m_offset);
}

RectF ScrollArea::verticalScrollbarRect() const
{
    if (!m_verticalScrollbarVisible)
        return {};

    const auto scale = m_viewportSize.height() / m_contentsSize.height();
    const auto handleHeight = scale * m_viewportSize.height();
    const auto handleY = scale * -m_offset.y;

    const auto topLeft = glm::vec2{m_viewportSize.width() + kScrollbarSpacing, handleY};
    const auto topRight = topLeft + glm::vec2{m_verticalScrollbarWidth - 2.0f * kScrollbarSpacing, handleHeight};

    return RectF{topLeft, topRight};
}

RectF ScrollArea::horizontalScrollbarRect() const
{
    if (!m_horizontalScrollbarVisible)
        return {};

    const auto scale = m_viewportSize.width() / m_contentsSize.width();
    const auto handleWidth = scale * m_viewportSize.width();
    const auto handleX = scale * -m_offset.x;

    const auto topLeft = glm::vec2{handleX, m_viewportSize.height() + kScrollbarSpacing};
    const auto topRight = topLeft + glm::vec2{handleWidth, m_horizontalScrollbarHeight - 2.0f * kScrollbarSpacing};

    return RectF{topLeft, topRight};
}

void ScrollArea::paintContents(Painter *painter, const glm::vec2 &pos, int depth) const
{
    Gizmo::paintContents(painter, pos, depth);

    if (const auto rect = verticalScrollbarRect())
    {
        const auto color = [this] {
            if (m_dragState == DragState::VerticalScrollbar)
                return scrollbarPressedColor;
            if (m_verticalScrollbarHovered)
                return scrollbarHoveredColor;
            return scrollbarColor;
        }();
        painter->setColor(color);
        painter->fillRect(RectF{rect.topLeft() + pos, rect.bottomRight() + pos}, depth);
    }

    if (const auto rect = horizontalScrollbarRect())
    {
        const auto color = [this] {
            if (m_dragState == DragState::HorizontalScrollbar)
                return scrollbarPressedColor;
            if (m_horizontalScrollbarHovered)
                return scrollbarHoveredColor;
            return scrollbarColor;
        }();
        painter->setColor(scrollbarColor);
        painter->fillRect(RectF{rect.topLeft() + pos, rect.bottomRight() + pos}, depth);
    }
}

void ScrollArea::paintChildren(Painter *painter, const glm::vec2 &pos, int depth) const
{
    const RectF prevClipRect = painter->clipRect();
    const auto clipRect = RectF{pos, m_viewportSize};
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

Icon::Icon(std::string_view source, Gizmo *parent)
    : Gizmo(parent)
{
    setSource(source);
}

void Icon::setSource(std::string_view source)
{
    if (source == m_source)
        return;
    m_source = source;
    if (const auto *image = findOrCreateImage(source))
        setSize(SizeF{image->size()});
}

void Icon::paintContents(Painter *painter, const glm::vec2 &pos, int depth) const
{
    Gizmo::paintContents(painter, pos, depth);
    painter->setColor(color);
    painter->drawIcon(pos, m_source, depth);
}

EventManager::EventManager() = default;

void EventManager::setRoot(Gizmo *root)
{
    m_root = root;
}

bool EventManager::handleMouseButton(MouseButton button, MouseAction action, const glm::vec2 &pos, Modifier mods)
{
    if (button != MouseButton::Left)
        return false;
    bool accepted = false;
    switch (action)
    {
    case MouseAction::Press: {
        auto *target =
            m_root->findChildAt(pos, [](Gizmo *gizmo, const glm::vec2 &pos) { return gizmo->handleMousePress(pos); });
        if (target)
        {
            // found a gizmo that accepts the mouse press, will get mouse move and the mouse release event
            setMouseEventTarget(target);
            accepted = true;
        }
        break;
    }
    case MouseAction::Release: {
        if (m_mouseEventTarget)
        {
            m_mouseEventTarget->handleMouseRelease(pos - m_mouseEventTarget->globalPosition());
            setMouseEventTarget(nullptr);
            accepted = true;
        }
        break;
    }
    }
    return accepted;
}

void EventManager::setMouseEventTarget(Gizmo *target)
{
    if (m_mouseEventTarget)
    {
        m_aboutToBeDestroyedConnection.disconnect();
    }
    m_mouseEventTarget = target;
    if (m_mouseEventTarget)
    {
        m_aboutToBeDestroyedConnection =
            m_mouseEventTarget->aboutToBeDestroyedSignal.connect([this] { setMouseEventTarget(nullptr); });
    }
}

bool EventManager::handleMouseMove(const glm::vec2 &pos)
{
    bool accepted = false;
#if 0
    auto *underCursor = m_root->findChildAt(pos, [](Gizmo *gizmo, const glm::vec2 &) { return gizmo->hoverable(); });
    if (m_underCursor != underCursor)
    {
        if (m_underCursor)
            m_underCursor->handleHoverLeave();
        m_underCursor = underCursor;
        if (m_underCursor)
            m_underCursor->handleHoverEnter();
    }
#endif

    m_root->findChildAt(pos, [this](Gizmo *gizmo, const glm::vec2 &pos) {
        if (gizmo->hasMouseTracking() && gizmo != m_mouseEventTarget)
            gizmo->handleMouseMove(pos);
        return false;
    });
    if (m_mouseEventTarget)
    {
        m_mouseEventTarget->handleMouseMove(pos - m_mouseEventTarget->globalPosition());
        accepted = true;
    }
    return accepted;
}

bool EventManager::handleMouseWheel(const glm::vec2 &mousePos, const glm::vec2 &wheelOffset)
{
    const auto *target = m_root->findChildAt(
        mousePos, [&wheelOffset](Gizmo *gizmo, const glm::vec2 &) { return gizmo->handleMouseWheel(wheelOffset); });
    return target != nullptr;
}

} // namespace ui
