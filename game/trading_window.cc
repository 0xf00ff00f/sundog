#include "trading_window.h"

#include "universe.h"
#include "market_snapshot_gizmo.h"
#include "market_item_details_gizmo.h"
#include "style_settings.h"

using namespace ui;

TradingWindow::TradingWindow(const World *world, Ship *ship, Gizmo *parent)
    : Column(parent)
    , m_world(world)
    , m_ship(ship)
{
    setFillBackground(true);
    backgroundColor = glm::vec4{0.0f, 0.0f, 0.0f, 0.75f};
    setMargins(4.0f);

    m_title = appendChild<ui::Text>();
    m_title->setFont(g_styleSettings.titleFont);
    m_title->color = g_styleSettings.accentColor;

    auto *marketName = appendChild<ui::Text>();
    marketName->setFont(g_styleSettings.normalFont);
    marketName->color = g_styleSettings.baseColor;

    appendChild<Rectangle>(1.0f, 40.0f);

    auto *marketRow = appendChild<ui::Row>();
    marketRow->setSpacing(40.0f);

    m_marketSnapshot = marketRow->appendChild<MarketSnapshotGizmo>(world, ship);
    m_marketItemDetails = marketRow->appendChild<MarketItemDetailsGizmo>(world, ship);

    m_marketSnapshot->itemSelectedSignal.connect(
        [this](const MarketItem *item) { m_marketItemDetails->setItem(item); });

    m_title->setText(m_world->name);
    marketName->setText(m_world->marketName);
}
