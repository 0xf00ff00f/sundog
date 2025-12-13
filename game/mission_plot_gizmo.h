#pragma once

#include "universe.h"

#include <base/gui.h>
#include <base/glhelpers.h>
#include <base/image.h>

class MissionTable;

class MissionPlotGizmo : public ui::Gizmo
{
public:
    explicit MissionPlotGizmo(const MissionTable *missionTable, Gizmo *parent = nullptr);

    void paintContents(Painter *painter, const glm::vec2 &pos, int depth) const override;
    bool handleMousePress(const glm::vec2 &pos) override;
    void handleMouseRelease(const glm::vec2 &pos) override;
    void handleMouseMove(const glm::vec2 &pos) override;

    std::optional<MissionPlan> missionPlan() const { return m_missionPlan; }

    muslots::Signal<> missionPlanChangedSignal;

private:
    void updateMissionPlan(const glm::vec2 &pos);

    const MissionTable *m_missionTable{nullptr};
    Image32 m_plotImage;
    gl::Texture m_plotTexture;
    std::optional<MissionPlan> m_missionPlan;
    std::optional<glm::vec2> m_selectedPoint;
};
