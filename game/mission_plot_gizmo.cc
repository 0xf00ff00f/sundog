#include "mission_plot_gizmo.h"

#include "mission_plot.h"

#include <mdspan>

MissionPlotGizmo::MissionPlotGizmo(const MissionTable *missionTable, Gizmo *parent)
    : ui::Gizmo(parent)
    , m_missionTable(missionTable)
    , m_plotImage(createMissionPlot(*missionTable))
    , m_plotTexture(m_plotImage)
{
    m_size = SizeF{m_plotImage.size()};

    m_plotTexture.setMinificationFilter(gl::Texture::Filter::Linear);
    m_plotTexture.setMagnificationFilter(gl::Texture::Filter::Linear);
    m_plotTexture.setWrapModeS(gl::Texture::WrapMode::ClampToEdge);
    m_plotTexture.setWrapModeT(gl::Texture::WrapMode::ClampToEdge);

    setFillBackground(true);
    backgroundColor = glm::vec4{1.0};
}

void MissionPlotGizmo::paintContents(Painter *painter, const glm::vec2 &pos, int depth) const
{
    painter->setColor(glm::vec4{1.0f});
    painter->drawSprite(&m_plotTexture, pos, glm::vec2{0.0f, 1.0f}, pos + glm::vec2{m_size.width(), m_size.height()},
                        glm::vec2{1.0f, 0.0f}, depth);
    if (m_selectedPoint)
    {
        painter->setColor(glm::vec4{0.0f, 0.0f, 0.0f, 1.0f});
        const auto x = pos.x + m_selectedPoint->x;
        const auto y = pos.y + m_selectedPoint->y;
        painter->strokePolyline(std::array{glm::vec2{x, pos.y}, glm::vec2{x, pos.y + m_size.height()}}, 1.0f, false,
                                depth + 1);
        painter->strokePolyline(std::array{glm::vec2{pos.x, y}, glm::vec2{pos.x + m_size.width(), y}}, 1.0f, false,
                                depth + 1);
    }
}

bool MissionPlotGizmo::handleMousePress(const glm::vec2 &pos)
{
    updateMissionPlan(pos);
    return true;
}

void MissionPlotGizmo::handleMouseRelease(const glm::vec2 &pos) {}

void MissionPlotGizmo::handleMouseMove(const glm::vec2 &pos)
{
    updateMissionPlan(pos);
}

void MissionPlotGizmo::updateMissionPlan(const glm::vec2 &pos)
{
    auto missionPlan = [this, &pos]() -> std::optional<MissionPlan> {
        const auto arrivalIndex = static_cast<int>(((m_plotImage.height() - 1.0 - pos.y) / m_plotImage.height()) *
                                                   m_missionTable->arrivals.size());
        if (arrivalIndex < 0 || arrivalIndex >= m_missionTable->arrivals.size())
            return {};

        const auto departureIndex = static_cast<int>((pos.x / m_plotImage.width()) * m_missionTable->departures.size());
        if (departureIndex < 0 || departureIndex >= m_missionTable->departures.size())
            return {};

        auto orbits = std::mdspan(m_missionTable->transferOrbits.data(), m_missionTable->arrivals.size(),
                                  m_missionTable->departures.size());
        const auto &orbit = orbits[arrivalIndex, departureIndex];
        if (!orbit.has_value())
            return {};

        const auto [timeArrival, posArrival, velWorldArrival] = m_missionTable->arrivals[arrivalIndex];
        const auto [timeDeparture, posDeparture, velWorldDeparture] = m_missionTable->departures[departureIndex];

        const auto orbitalElements = orbitalElementsFromStateVector(posArrival, orbit->velArrival, timeArrival);

        MissionPlan missionPlan{.origin = m_missionTable->origin(),
                                .destination = m_missionTable->destination(),
                                .departureTime = timeDeparture,
                                .arrivalTime = timeArrival};
        missionPlan.orbit.setElements(orbitalElements);
        missionPlan.deltaVDeparture = orbit->deltaVDeparture;
        missionPlan.deltaVArrival = orbit->deltaVArrival;
        return missionPlan;
    }();

    m_missionPlan = missionPlan;
    if (m_missionPlan)
        m_selectedPoint = pos;
    else
        m_selectedPoint = {};
    missionPlanChangedSignal();
}
