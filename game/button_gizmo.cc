#include "button_gizmo.h"

#include "style_settings.h"

using namespace ui;

ButtonGizmo::ButtonGizmo(std::string_view text, ui::Gizmo *parent)
    : Rectangle(parent)
    , m_text(appendChild<Text>(g_styleSettings.normalFont, text))
{
    m_text->setAlign(Align::VerticalCenter | Align::HorizontalCenter);
    m_text->color = glm::vec4{0.0, 0.0, 0.0, 1.0};
    setHoverable(true);
    setFillBackground(true);
    backgroundColor = glm::vec4{1.0, 0.75, 0.0, 1.0};
}

bool ButtonGizmo::handleMousePress(const glm::vec2 & /* pos */)
{
    return true;
}

void ButtonGizmo::handleMouseRelease(const glm::vec2 & /* pos */)
{
    clickedSignal();
}

void ButtonGizmo::handleHoverEnter()
{
    backgroundColor = glm::vec4{1.0, 1.0, 0.5, 1.0};
}

void ButtonGizmo::handleHoverLeave()
{
    backgroundColor = glm::vec4{1.0, 0.75, 0.0, 1.0};
}
