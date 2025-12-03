#pragma once

#include <base/gui.h>

class Ship;
class MarketItem;
class TableGizmo;
class World;
class ButtonGizmo;

class MarketItemDetailsGizmo : public ui::Column
{
public:
    explicit MarketItemDetailsGizmo(const World *world, Ship *ship, ui::Gizmo *parent = nullptr);

    void setItem(const MarketItem *item);

private:
    const World *m_world{nullptr};
    Ship *m_ship{nullptr};
    const MarketItem *m_item{nullptr};
    ui::Text *m_nameText{nullptr};
    ui::Text *m_sectorText{nullptr};
    ui::MultiLineText *m_descriptionText{nullptr};
    ui::Text *m_sellPriceText{nullptr};
    ui::Text *m_buyPriceText{nullptr};
    TableGizmo *m_exporterTable{nullptr};
    TableGizmo *m_importerTable{nullptr};
    ButtonGizmo *m_sellButton{nullptr};
    ButtonGizmo *m_buyButton{nullptr};
};
