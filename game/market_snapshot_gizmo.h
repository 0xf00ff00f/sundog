#pragma once

#include <base/gui.h>

class World;
class Ship;
class TableGizmo;
class MarketItem;

class MarketSnapshotGizmo : public ui::Column
{
public:
    explicit MarketSnapshotGizmo(const World *world, const Ship *ship, ui::Gizmo *parent = nullptr);

    muslots::Signal<const MarketItem *> itemSelectedSignal;

private:
    void initialize();

    const World *m_world{nullptr};
    const Ship *m_ship{nullptr};
    TableGizmo *m_tableGizmo{nullptr};
};
