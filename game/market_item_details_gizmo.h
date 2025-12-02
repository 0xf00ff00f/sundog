#pragma once

#include <base/gui.h>

class Universe;
class MarketItem;
class TableGizmo;
class World;

class MarketItemDetailsGizmo : public ui::Column
{
public:
    explicit MarketItemDetailsGizmo(const Universe *universe, ui::Gizmo *parent = nullptr);

    void initializeFrom(const World *world, const MarketItem *item);

private:
    const Universe *m_universe{nullptr};
    ui::Text *m_nameText{nullptr};
    ui::Text *m_sectorText{nullptr};
    ui::MultiLineText *m_descriptionText{nullptr};
    ui::Text *m_sellPriceText{nullptr};
    ui::Text *m_buyPriceText{nullptr};
    TableGizmo *m_exporterTable{nullptr};
    TableGizmo *m_importerTable{nullptr};
};
