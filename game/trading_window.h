#pragma once

#include <base/gui.h>

class World;
class Ship;
class MarketSnapshotGizmo;
class MarketItemDetailsGizmo;

class TradingWindow : public ui::Column
{
public:
    explicit TradingWindow(const World *world, Ship *ship, ui::Gizmo *parent = nullptr);

private:
    void initialize();

    const World *m_world{nullptr};
    Ship *m_ship{nullptr};
    ui::Text *m_title{nullptr};
    MarketSnapshotGizmo *m_marketSnapshot{nullptr};
    MarketItemDetailsGizmo *m_marketItemDetails{nullptr};
};
