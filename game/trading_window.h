#pragma once

#include <base/gui.h>

class Universe;
class World;
class MarketSnapshotGizmo;
class MarketItemDetailsGizmo;

class TradingWindow : public ui::Row
{
public:
    explicit TradingWindow(const Universe *universe, ui::Gizmo *parent = nullptr);

    void initializeFrom(const World *world);

private:
    const Universe *m_universe{nullptr};
    MarketSnapshotGizmo *m_marketSnapshot{nullptr};
    MarketItemDetailsGizmo *m_marketItemDetails{nullptr};
};
