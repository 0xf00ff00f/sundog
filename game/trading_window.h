#pragma once

#include <base/gui.h>

class Universe;
class World;
class MarketSnapshotGizmo;
class MarketItemDetailsGizmo;

class TradingWindow : public ui::Column
{
public:
    explicit TradingWindow(const Universe *universe, ui::Gizmo *parent = nullptr);

    void initializeFrom(const World *world);

private:
    const Universe *m_universe{nullptr};
    const World *m_world{nullptr};
    ui::Text *m_title{nullptr};
    MarketSnapshotGizmo *m_marketSnapshot{nullptr};
    MarketItemDetailsGizmo *m_marketItemDetails{nullptr};
};
