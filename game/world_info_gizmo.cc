#include "world_info_gizmo.h"

#include "universe.h"
#include "style_settings.h"

namespace
{
constexpr auto kTotalWidth = 400.0f;
}

WorldInfoGizmo::WorldInfoGizmo(ui::Gizmo *parent)
    : ui::Column(parent)
{
    setMinimumWidth(kTotalWidth);

    setFillBackground(true);
    setMargins(20.0f);
    backgroundColor = glm::vec4{0.0f, 0.0f, 0.0f, 0.75f};

    m_nameText = appendChild<ui::Text>();
    m_nameText->setFont(g_styleSettings.titleFont);
    m_nameText->color = g_styleSettings.accentColor;
}

void WorldInfoGizmo::setWorld(const World *world)
{
    if (world == m_world)
        return;

    m_world = world;

    m_nameText->setText(m_world->name);
}
