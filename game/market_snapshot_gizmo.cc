#include "market_snapshot_gizmo.h"

#include "universe.h"

using namespace ui;

namespace
{

constexpr auto kGoodsColumnWidth = 200.0f;
constexpr auto kPriceColumnWidth = 120.0f;

ui::Gizmo *appendPaddedText(float width, Align align, std::string_view text, ui::Gizmo *parent)
{
    constexpr auto kFont = "DejaVuSans.ttf";
    constexpr auto kMargin = 4.0f;

    auto *container = parent->appendChild<Column>();
    container->setMargins(kMargin);
    container->setMinimumWidth(width);

    auto *textGizmo = container->appendChild<Text>();
    textGizmo->setAlign(align);
    textGizmo->setFont(Font{kFont, 20.0f, 0});
    textGizmo->setText(text);
    textGizmo->color = glm::vec4{1.0f};

    return container;
}

}; // namespace

MarketSnapshotGizmo::MarketSnapshotGizmo(ui::Gizmo *parent)
    : ui::Column(parent)
{
    setFillBackground(true);
    backgroundColor = glm::vec4{0.0f, 0.0f, 0.0f, 0.75f};
    setMargins(4.0f);

    m_headerRow = appendChild<ui::Row>();
    appendPaddedText(kGoodsColumnWidth, Align::Left, "Goods", m_headerRow);
    appendPaddedText(kPriceColumnWidth, Align::Right, "Buy", m_headerRow);
    appendPaddedText(kPriceColumnWidth, Align::Right, "Sell", m_headerRow);

    const auto headerWidth = kGoodsColumnWidth + 2.0f * kPriceColumnWidth + m_headerRow->spacing() * 2;

    constexpr auto kScrollbarWidth = 8.0f;

    m_scrollArea = appendChild<ui::ScrollArea>(headerWidth + kScrollbarWidth, 200.0f);
    m_scrollArea->setVerticalScrollbarWidth(kScrollbarWidth);

    m_itemList = m_scrollArea->appendChild<ui::Column>();
}

void MarketSnapshotGizmo::initializeFrom(const World *world)
{
    m_itemList->clear();

    const auto &items = world->marketItems();

    for (const auto *sector : world->universe()->marketSectors())
    {
        auto filteredItems =
            items | std::views::filter([sector](const auto &item) { return item.description->sector == sector; });
        if (!filteredItems.empty())
        {
            auto *row = m_itemList->appendChild<ui::Row>();
            appendPaddedText(kGoodsColumnWidth, Align::Left, sector->name, row);

            for (const auto &item : filteredItems)
            {
                auto *row = m_itemList->appendChild<ui::Row>();
                appendPaddedText(kGoodsColumnWidth, Align::Left, item.description->name, row);
                appendPaddedText(kPriceColumnWidth, Align::Right, std::to_string(item.buyPrice), row);
                appendPaddedText(kPriceColumnWidth, Align::Right, std::to_string(item.sellPrice), row);
            }
        }
    }
}
