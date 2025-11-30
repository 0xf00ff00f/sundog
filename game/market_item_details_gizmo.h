#pragma once

#include <base/gui.h>

class Universe;
class MarketItemInfo;
class TableGizmo;

class MarketItemDetailsGizmo : public ui::Column
{
public:
    explicit MarketItemDetailsGizmo(const Universe *universe, ui::Gizmo *parent = nullptr);

    void initializeFrom(const MarketItemInfo *marketItem);

private:
    const Universe *m_universe{nullptr};
    ui::Text *m_nameText{nullptr};
    ui::Text *m_sectorText{nullptr};
    ui::MultiLineText *m_descriptionText{nullptr};
};
