#include "trading_window.h"

#include "market_snapshot_gizmo.h"
#include "market_item_details_gizmo.h"

using namespace ui;

TradingWindow::TradingWindow(const Universe *universe, Gizmo *parent)
    : Row(parent)
    , m_universe(universe)
{
    setFillBackground(true);
    backgroundColor = glm::vec4{0.0f, 0.0f, 0.0f, 0.75f};
    setMargins(4.0f);

    setSpacing(20.0f);

    m_marketSnapshot = appendChild<MarketSnapshotGizmo>(universe);
    m_marketItemDetails = appendChild<MarketItemDetailsGizmo>(universe);

    m_marketSnapshot->itemSelectedSignal.connect(
        [this](const MarketItemInfo *item) { m_marketItemDetails->initializeFrom(item); });
}

void TradingWindow::initializeFrom(const World *world)
{
    m_marketSnapshot->initializeFrom(world);
}
