#pragma once

#include <base/gui.h>

class World;
class Ship;
class TableGizmo;
class MarketItem;

class MarketSnapshotGizmo : public ui::Column
{
public:
    explicit MarketSnapshotGizmo(const World *world, Ship *ship, ui::Gizmo *parent = nullptr);
    ~MarketSnapshotGizmo() override;

    muslots::Signal<const MarketItem *> itemSelectedSignal;

private:
    void initialize();

    const World *m_world{nullptr};
    Ship *m_ship{nullptr};
    TableGizmo *m_tableGizmo{nullptr};
    muslots::Connection m_cargoChangedConnection;
};
