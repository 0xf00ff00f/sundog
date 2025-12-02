#pragma once

#include <base/gui.h>

class Universe;
class World;
class TableGizmo;
class MarketItem;

class MarketSnapshotGizmo : public ui::Column
{
public:
    explicit MarketSnapshotGizmo(const Universe *universe, ui::Gizmo *parent = nullptr);

    void initializeFrom(const World *world);

    muslots::Signal<const MarketItem *> itemSelectedSignal;

private:
    const Universe *m_universe{nullptr};
    const World *m_world{nullptr};
    TableGizmo *m_tableGizmo{nullptr};
};
