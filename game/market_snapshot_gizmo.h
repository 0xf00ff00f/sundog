#pragma once

#include <base/gui.h>

class World;
class TableGizmo;

class MarketSnapshotGizmo : public ui::Column
{
public:
    explicit MarketSnapshotGizmo(ui::Gizmo *parent);

    void initializeFrom(const World *world);

private:
    TableGizmo *m_tableGizmo{nullptr};
};
