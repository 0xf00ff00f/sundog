#include "trading_window.h"

#include "universe.h"
#include "market_snapshot_gizmo.h"
#include "market_item_details_gizmo.h"
#include "style_settings.h"

using namespace ui;

TradingWindow::TradingWindow(const Universe *universe, Gizmo *parent)
    : Column(parent)
    , m_universe(universe)
{
    setFillBackground(true);
    backgroundColor = glm::vec4{0.0f, 0.0f, 0.0f, 0.75f};
    setMargins(4.0f);

    setSpacing(20.0f);

    m_title = appendChild<ui::Text>();
    m_title->setFont(g_styleSettings.titleFont);

    auto *marketRow = appendChild<ui::Row>();
    marketRow->setSpacing(20.0f);

    m_marketSnapshot = marketRow->appendChild<MarketSnapshotGizmo>(universe);
    m_marketItemDetails = marketRow->appendChild<MarketItemDetailsGizmo>(universe);

    m_marketSnapshot->itemSelectedSignal.connect(
        [this](const MarketItemInfo *item) { m_marketItemDetails->initializeFrom(item); });
}

void TradingWindow::initializeFrom(const World *world)
{
    m_title->setText(world->name());
    m_marketSnapshot->initializeFrom(world);
}
