#include "ship_info_gizmo.h"

#include "universe.h"
#include "style_settings.h"
#include "table_gizmo.h"

using namespace ui;

namespace
{
constexpr auto kTotalWidth = 400.0f;
}

ShipInfoGizmo::ShipInfoGizmo(Gizmo *parent)
    : Column(parent)
{
    setMinimumWidth(kTotalWidth);

    setFillBackground(true);
    setMargins(20.0f);
    backgroundColor = glm::vec4{0.0f, 0.0f, 0.0f, 0.75f};

    m_nameText = appendChild<Text>();
    m_nameText->setFont(g_styleSettings.titleFont);
    m_nameText->color = g_styleSettings.accentColor;

    m_statusText = appendChild<Text>();
    m_statusText->setFont(g_styleSettings.normalFont);
    m_statusText->color = g_styleSettings.baseColor;
}

ShipInfoGizmo::~ShipInfoGizmo()
{
    m_stateChangedConnection.disconnect();
}

void ShipInfoGizmo::setShip(Ship *ship)
{
    if (ship == m_ship)
        return;

    if (m_ship)
        m_stateChangedConnection.disconnect();

    m_ship = ship;

    if (m_ship)
        m_stateChangedConnection = m_ship->stateChangedSignal.connect([this](Ship::State) { updateText(); });

    updateText();
}

void ShipInfoGizmo::updateText()
{
    if (!m_ship)
        return;

    m_nameText->setText(m_ship->shipClass()->name);

    switch (m_ship->state())
    {
    case Ship::State::Docked: {
        m_statusText->setText(std::format("Docked at {}", m_ship->world()->name));
        break;
    }
    case Ship::State::InTransit:
        m_statusText->setText(std::format("En route to {}", m_ship->missionPlan()->destination->name));
        break;
    }
}
