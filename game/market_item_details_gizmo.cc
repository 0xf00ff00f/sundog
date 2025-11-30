#include "market_item_details_gizmo.h"

#include "universe.h"
#include "style_settings.h"

using namespace ui;

MarketItemDetailsGizmo::MarketItemDetailsGizmo(const Universe *universe, Gizmo *parent)
    : Column(parent)
    , m_universe(universe)
{
    m_nameText = appendChild<ui::Text>();
    m_nameText->setFont(g_styleSettings.titleFont);

    m_sectorText = appendChild<ui::Text>();
    m_sectorText->setFont(g_styleSettings.normalFont);

    m_descriptionText = appendChild<ui::MultiLineText>();
    m_descriptionText->setFont(g_styleSettings.smallFont);
    m_descriptionText->setLineWidth(200.0f);
}

void MarketItemDetailsGizmo::initializeFrom(const MarketItemDescription *marketItem)
{
    m_nameText->setText(marketItem->name);
    m_sectorText->setText(marketItem->sector->name);
    m_descriptionText->setText(marketItem->description);
}
