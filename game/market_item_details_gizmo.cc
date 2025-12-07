#include "market_item_details_gizmo.h"

#include "universe.h"
#include "style_settings.h"
#include "table_gizmo.h"
#include "button_gizmo.h"
#include "util.h"

#include <algorithm>

using namespace ui;

namespace
{

constexpr auto kTotalWidth = 400.0f;

void addSeparator(Gizmo *parent, float width, const glm::vec4 &color)
{
    auto *separator = parent->appendChild<Rectangle>(width, 1.0f);
    separator->setFillBackground(true);
    separator->backgroundColor = color;
}

} // namespace

MarketItemDetailsGizmo::MarketItemDetailsGizmo(const World *world, Ship *ship, Gizmo *parent)
    : Column(parent)
    , m_world(world)
    , m_ship(ship)
{
    setMinimumWidth(kTotalWidth);

    m_nameText = appendChild<ui::Text>();
    m_nameText->setFont(g_styleSettings.titleFont);
    m_nameText->color = g_styleSettings.accentColor;

    addSeparator(this, kTotalWidth, g_styleSettings.baseColor);

    m_sectorText = appendChild<ui::Text>();
    m_sectorText->setFont(g_styleSettings.normalFont);
    m_sectorText->color = g_styleSettings.baseColor;

    appendChild<ui::Rectangle>(0.0f, 20.0f);

    auto *priceRow = appendChild<Row>();
    priceRow->setSpacing(20.0f);

    auto *sellColumn = priceRow->appendChild<ui::Column>();
    auto *sellLabel = sellColumn->appendChild<Text>(g_styleSettings.smallFont, "Sell to market");
    sellLabel->color = g_styleSettings.baseColor;
    addSeparator(sellColumn, (kTotalWidth - 20.0f) / 2.0f, g_styleSettings.baseColor);
    m_sellPriceText = sellColumn->appendChild<Text>();
    m_sellPriceText->setAlign(Align::VerticalCenter | Align::Right);
    m_sellPriceText->setFont(g_styleSettings.normalFont);
    m_sellPriceText->color = g_styleSettings.accentColor;

    auto *buyColumn = priceRow->appendChild<ui::Column>();
    auto *buyLabel = buyColumn->appendChild<Text>(g_styleSettings.smallFont, "Buy from market");
    buyLabel->color = g_styleSettings.baseColor;
    addSeparator(buyColumn, (kTotalWidth - 20.0f) / 2.0f, g_styleSettings.baseColor);
    m_buyPriceText = buyColumn->appendChild<Text>();
    m_buyPriceText->setAlign(Align::VerticalCenter | Align::Right);
    m_buyPriceText->setFont(g_styleSettings.normalFont);
    m_buyPriceText->color = g_styleSettings.accentColor;

    appendChild<ui::Rectangle>(0.0f, 20.0f);

    auto *descriptionScrollArea = appendChild<ui::ScrollArea>();
    descriptionScrollArea->setSize(kTotalWidth, 280.0f);

    auto *descriptionColumn = descriptionScrollArea->appendChild<ui::Column>();

    m_descriptionText = descriptionColumn->appendChild<ui::MultiLineText>();
    m_descriptionText->setFont(g_styleSettings.smallFont);
    m_descriptionText->color = g_styleSettings.baseColor;
    m_descriptionText->setLineWidth(kTotalWidth - descriptionScrollArea->verticalScrollbarWidth());

    descriptionColumn->appendChild<ui::Rectangle>(0.0f, 20.0f);

    constexpr auto kPriceColumnWidth = 120.0f;

    m_importerTable = descriptionColumn->appendChild<TableGizmo>(2);
    m_importerTable->setHeader("Consumed by:");

    m_importerTable->setColumnWidth(0, kTotalWidth -
                                           (descriptionScrollArea->verticalScrollbarWidth() + kPriceColumnWidth));

    m_importerTable->setColumnWidth(1, kPriceColumnWidth);
    m_importerTable->setColumnAlign(1, Align::Right);

    descriptionColumn->appendChild<ui::Rectangle>(0.0f, 20.0f);

    m_exporterTable = descriptionColumn->appendChild<TableGizmo>(2);
    m_exporterTable->setHeader("Also produced by:");

    m_exporterTable->setColumnWidth(0, kTotalWidth -
                                           (descriptionScrollArea->verticalScrollbarWidth() + kPriceColumnWidth));

    m_exporterTable->setColumnWidth(1, kPriceColumnWidth);
    m_exporterTable->setColumnAlign(1, Align::Right);

    auto *buttonRow = appendChild<ui::Row>();
    buttonRow->setAlign(Align::VerticalCenter | Align::Right);

    m_sellButton = buttonRow->appendChild<ButtonGizmo>("Sell");
    m_sellButton->setSize(80, 30);

    m_buyButton = buttonRow->appendChild<ButtonGizmo>("Buy");
    m_buyButton->setSize(80, 30);

    m_sellButton->clickedSignal.connect([this] {
        if (m_item)
            m_ship->changeCargo(m_item, -1);
    });

    m_buyButton->clickedSignal.connect([this] {
        if (m_item)
            m_ship->changeCargo(m_item, 1);
    });
}

void MarketItemDetailsGizmo::setItem(const MarketItem *item)
{
    if (item == m_item)
        return;

    m_item = item;

    m_nameText->setText(m_item->name);
    m_sectorText->setText(m_item->sector->name);
    m_descriptionText->setText(m_item->description);

    const auto *price = m_world->findMarketItemPrice(m_item);
    m_sellPriceText->setText(formatCredits(price ? price->buyPrice : 0));
    m_buyPriceText->setText(formatCredits(price ? price->sellPrice : 0));

    struct WorldPrice
    {
        const World *world;
        uint64_t price;
    };
    std::vector<WorldPrice> buyPrices, sellPrices;
    for (const auto *world : m_world->universe()->worlds())
    {
        if (world == m_world)
            continue;
        const auto *price = world->findMarketItemPrice(m_item);
        if (price)
        {
            if (price->buyPrice)
                buyPrices.emplace_back(world, price->buyPrice);
            if (price->sellPrice)
                sellPrices.emplace_back(world, price->sellPrice);
        }
    }
    std::ranges::sort(buyPrices, std::ranges::greater{}, &WorldPrice::price);
    std::ranges::sort(sellPrices, std::ranges::less{}, &WorldPrice::price);

    m_importerTable->clearRows();
    for (const auto &[world, price] : buyPrices | std::views::take(4))
        m_importerTable->appendRow(world->name, price);
    m_importerTable->setVisibleRowCount(m_importerTable->rowCount());

    m_exporterTable->clearRows();
    for (const auto &[world, price] : sellPrices | std::views::take(4))
        m_exporterTable->appendRow(world->name, price);
    m_exporterTable->setVisibleRowCount(m_exporterTable->rowCount());
}
