#pragma once

#include <base/gui.h>

class World;

class MarketSnapshotGizmo : public ui::Column
{
public:
    explicit MarketSnapshotGizmo(ui::Gizmo *parent);

    void initializeFrom(const World *world);

private:
    ui::Row *m_headerRow{nullptr};
    ui::ScrollArea *m_scrollArea{nullptr};
    ui::Column *m_itemList{nullptr};
};
