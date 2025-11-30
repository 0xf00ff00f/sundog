#include "market_snapshot_gizmo.h"

#include "universe.h"
#include "table_gizmo.h"
#include "style_settings.h"

using namespace ui;

namespace
{
constexpr auto kBlack = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f};
}

MarketSnapshotGizmo::MarketSnapshotGizmo(const Universe *universe, Gizmo *parent)
    : Column(parent)
    , m_universe(universe)
{
    m_tableGizmo = appendChild<TableGizmo>(3);

    m_tableGizmo->setVisibleRowCount(18);
    m_tableGizmo->setHeaderSeparatorColor(g_styleSettings.baseColor);

    m_tableGizmo->setColumnWidth(0, 300.0f);
    m_tableGizmo->setColumnAlign(0, Align::Left);

    m_tableGizmo->setColumnWidth(1, 120.0f);
    m_tableGizmo->setColumnAlign(1, Align::Right);

    m_tableGizmo->setColumnWidth(2, 120.0f);
    m_tableGizmo->setColumnAlign(2, Align::Right);

    m_tableGizmo->headerRow()->setTextColor(g_styleSettings.baseColor);
    m_tableGizmo->setHeader("Goods", "Buy", "Sell");

    m_tableGizmo->rowSelectedSignal.connect([this](const TableGizmoRow *row) {
        if (row)
        {
            const auto *item = std::any_cast<const MarketItemInfo *>(row->data());
            itemSelectedSignal(item);
        }
    });
}

void MarketSnapshotGizmo::initializeFrom(const World *world)
{
    m_tableGizmo->clearRows();

    const auto &items = world->marketItems();

    for (const auto *sector : m_universe->marketSectors())
    {
        auto filteredItems =
            items | std::views::filter([sector](const auto &item) { return item.info->sector == sector; });
        if (!filteredItems.empty())
        {
            auto *row = m_tableGizmo->appendRow(sector->name);
            row->setTextColor(g_styleSettings.accentColor);

            for (const auto &item : filteredItems)
            {
                auto *row = m_tableGizmo->appendRow(item.info->name, item.buyPrice, item.sellPrice);
                row->setHoverable(true);
                row->setHoveredColor(glm::vec4{1.0f, 1.0f, 1.0f, 0.25f});
                row->setSelectedColor(g_styleSettings.baseColor);
                row->setTextColor(g_styleSettings.baseColor);
                row->setSelectedTextColor(kBlack);
                row->setIndent(0, 20.0f);
                row->setSelectable(true);
                row->setData(item.info);
            }
        }
    }
}
