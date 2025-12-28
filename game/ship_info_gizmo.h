#pragma once

#include <base/gui.h>

class Ship;
class TableGizmo;

class ShipInfoGizmo : public ui::Column
{
public:
    explicit ShipInfoGizmo(ui::Gizmo *parent = nullptr);
    ~ShipInfoGizmo() override;

    void setShip(Ship *ship);

private:
    void updateText();

    Ship *m_ship{nullptr};
    ui::Text *m_nameText{nullptr};
    ui::Text *m_statusText{nullptr};
    muslots::Connection m_stateChangedConnection;
};
