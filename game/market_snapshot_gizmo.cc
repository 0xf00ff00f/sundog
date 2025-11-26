#include "market_snapshot_gizmo.h"

#include "universe.h"
#include "table_gizmo.h"

using namespace ui;

MarketSnapshotGizmo::MarketSnapshotGizmo(ui::Gizmo *parent)
    : ui::Column(parent)
{
    setFillBackground(true);
    backgroundColor = glm::vec4{0.0f, 0.0f, 0.0f, 0.75f};
    setMargins(4.0f);

    m_tableGizmo = appendChild<TableGizmo>(3);

    m_tableGizmo->setColumnWidth(0, 200.0f);
    m_tableGizmo->setColumnAlign(0, Align::Left);

    m_tableGizmo->setColumnWidth(1, 120.0f);
    m_tableGizmo->setColumnAlign(1, Align::Right);

    m_tableGizmo->setColumnWidth(2, 120.0f);
    m_tableGizmo->setColumnAlign(2, Align::Right);

    m_tableGizmo->setHeader("Goods", "Buy", "Sell");
}

void MarketSnapshotGizmo::initializeFrom(const World *world)
{
    m_tableGizmo->clearRows();

    const auto &items = world->marketItems();

    for (const auto *sector : world->universe()->marketSectors())
    {
        auto filteredItems =
            items | std::views::filter([sector](const auto &item) { return item.description->sector == sector; });
        if (!filteredItems.empty())
        {
            m_tableGizmo->appendRow(sector->name);
            for (const auto &item : filteredItems)
                m_tableGizmo->appendRow(item.description->name, item.buyPrice, item.sellPrice);
        }
    }
}
