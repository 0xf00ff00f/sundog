#pragma once

#include "universe.h"

#include <base/gui.h>

class MissionTable;
class MissionPlotGizmo;

class MissionPlanGizmo : public ui::Column
{
public:
    explicit MissionPlanGizmo(Ship *ship, const MissionTable *missionTable, ui::Gizmo *parent = nullptr);

    muslots::Signal<> confirmClickedSignal;

private:
    void updateTrajectoryValues();

    Ship *m_ship{nullptr};
    MissionPlotGizmo *m_missionPlotGizmo{nullptr};
    ui::Text *m_departureDateText{nullptr};
    ui::Text *m_arrivalDateText{nullptr};
    ui::Text *m_transitTimeText{nullptr};
    ui::Text *m_departureDeltaVText{nullptr};
    ui::Text *m_arrivalDeltaVText{nullptr};
    ui::Text *m_totalDeltaVText{nullptr};
};
