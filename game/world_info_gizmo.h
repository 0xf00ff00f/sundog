#pragma once

#include <base/gui.h>

class World;

class WorldInfoGizmo : public ui::Column
{
public:
    explicit WorldInfoGizmo(ui::Gizmo *parent = nullptr);

    void setWorld(const World *world);

private:
    const World *m_world{nullptr};
    ui::Text *m_nameText{nullptr};
};
