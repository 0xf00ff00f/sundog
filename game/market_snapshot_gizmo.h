#pragma once

#include <base/gui.h>

class World;
class TableGizmo;
class MarketItemDescription;

class MarketSnapshotGizmo : public ui::Column
{
public:
    explicit MarketSnapshotGizmo(ui::Gizmo *parent);

    void initializeFrom(const World *world);

    muslots::Signal<const MarketItemDescription *> itemSelectedSignal;

private:
    TableGizmo *m_tableGizmo{nullptr};
};
