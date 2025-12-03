#include "market_snapshot_gizmo.h"

#include "universe.h"
#include "table_gizmo.h"
#include "style_settings.h"

using namespace ui;

namespace
{
constexpr auto kBlack = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f};
}

MarketSnapshotGizmo::MarketSnapshotGizmo(const World *world, Ship *ship, Gizmo *parent)
    : Column(parent)
    , m_world(world)
    , m_ship(ship)
{
    m_tableGizmo = appendChild<TableGizmo>(4);

    m_tableGizmo->setVisibleRowCount(18);
    m_tableGizmo->setHeaderSeparatorColor(g_styleSettings.baseColor);

    m_tableGizmo->setColumnWidth(0, 100.0f);
    m_tableGizmo->setColumnAlign(0, Align::Left);

    m_tableGizmo->setColumnWidth(1, 250.0f);
    m_tableGizmo->setColumnAlign(1, Align::Left);

    m_tableGizmo->setColumnWidth(2, 100.0f);
    m_tableGizmo->setColumnAlign(2, Align::Right);

    m_tableGizmo->setColumnWidth(3, 100.0f);
    m_tableGizmo->setColumnAlign(3, Align::Right);

    m_tableGizmo->headerRow()->setTextColor(g_styleSettings.baseColor);
    m_tableGizmo->setHeader("Cargo", "Goods", "Buy", "Sell");

    m_tableGizmo->rowSelectedSignal.connect([this](const TableGizmoRow *row) {
        if (row)
        {
            const auto *item = std::any_cast<const MarketItem *>(row->data());
            itemSelectedSignal(item);
        }
    });

    initialize();

    m_cargoChangedConnection = m_ship->cargoChangedSignal.connect([this](const MarketItem *item) {
        auto *row = [this, item]() -> TableGizmoRow * {
            for (std::size_t i = 0; i < m_tableGizmo->rowCount(); ++i)
            {
                auto *row = m_tableGizmo->rowAt(i);
                auto rowData = row->data();
                if (rowData.has_value())
                {
                    const auto *rowItem = std::any_cast<const MarketItem *>(rowData);
                    if (rowItem == item)
                        return row;
                }
            }
            return nullptr;
        }();
        if (row)
            row->setValue(0, m_ship->cargo(item));
    });
}

MarketSnapshotGizmo::~MarketSnapshotGizmo()
{
    m_cargoChangedConnection.disconnect();
}

void MarketSnapshotGizmo::initialize()
{
    m_tableGizmo->clearRows();

    const auto &prices = m_world->marketItemPrices();

    for (const auto *sector : m_world->universe()->marketSectors())
    {
        auto filteredPrices =
            prices | std::views::filter([sector](const auto &price) { return price.item->sector == sector; });
        if (!filteredPrices.empty())
        {
            auto *row = m_tableGizmo->appendRow();
            row->setValue(1, sector->name);
            row->setTextColor(g_styleSettings.accentColor);

            for (const auto &price : filteredPrices)
            {
                const auto *item = price.item;
                const auto cargo = m_ship->cargo(item);
                auto *row = m_tableGizmo->appendRow(cargo, price.item->name, price.buyPrice, price.sellPrice);
                row->setHoverable(true);
                row->setHoveredColor(glm::vec4{1.0f, 1.0f, 1.0f, 0.25f});
                row->setSelectedColor(g_styleSettings.baseColor);
                row->setTextColor(g_styleSettings.baseColor);
                row->setSelectedTextColor(kBlack);
                row->setIndent(1, 20.0f);
                row->setSelectable(true);
                row->setData(price.item);
            }
        }
    }
}
