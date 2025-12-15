#include "mission_plot_gizmo.h"

#include "mission_plot.h"
#include "style_settings.h"

#include <mdspan>

using namespace ui;

MissionPlotGizmo::MissionPlotGizmo(const MissionTable *missionTable, Gizmo *parent)
    : Gizmo(parent)
    , m_font(g_styleSettings.smallFont)
    , m_missionTable(missionTable)
    , m_plotImage(createMissionPlot(*missionTable))
    , m_plotTexture(m_plotImage)
{
    m_margins = Margins{.left = m_font.pixelHeight, .right = 0, .top = 0, .bottom = m_font.pixelHeight};

    m_size = SizeF{Size{m_plotImage.width() + m_margins.left + m_margins.right,
                        m_plotImage.height() + m_margins.top + m_margins.bottom}};

    m_plotTexture.setMinificationFilter(gl::Texture::Filter::Linear);
    m_plotTexture.setMagnificationFilter(gl::Texture::Filter::Linear);
    m_plotTexture.setWrapModeS(gl::Texture::WrapMode::ClampToEdge);
    m_plotTexture.setWrapModeT(gl::Texture::WrapMode::ClampToEdge);
}

void MissionPlotGizmo::paintContents(Painter *painter, const glm::vec2 &pos, int depth) const
{
    using namespace std::literals::string_view_literals;

    const auto fm = FontMetrics{m_font};

    const auto fontHeight = m_font.pixelHeight;

    painter->setColor(glm::vec4{1.0f});
    painter->drawSprite(&m_plotTexture, pos + glm::vec2{m_margins.left, m_margins.top}, glm::vec2{0.0f, 1.0f},
                        pos + glm::vec2{m_margins.left + m_plotImage.width(), m_margins.top + m_plotImage.height()},
                        glm::vec2{1.0f, 0.0f}, depth);

    painter->setFont(m_font);
    painter->setColor(glm::vec4{1.0f});
    constexpr auto kArrivalText = "Arrival Date"sv;
    const auto leftTextHeight = fm.horizontalAdvance(kArrivalText);
    painter->drawText(pos + glm::vec2{m_margins.left - m_font.pixelHeight,
                                      m_margins.top + 0.5f * (m_plotImage.height() + leftTextHeight)},
                      kArrivalText, Painter::Rotation::Rotate90, depth);

    constexpr auto kDepartureText = "Departure Date"sv;
    const auto bottomTextWidth = fm.horizontalAdvance(kDepartureText);
    painter->drawText(pos + glm::vec2{m_margins.left + 0.5f * (m_plotImage.width() - bottomTextWidth),
                                      m_margins.top + m_plotImage.height()},
                      kDepartureText, depth);

    if (m_selectedPoint)
    {
        painter->setColor(glm::vec4{0.0f, 0.0f, 0.0f, 1.0f});
        const auto x = pos.x + m_selectedPoint->x + m_margins.left;
        const auto y = pos.y + m_selectedPoint->y + m_margins.top;
        painter->strokeLine(glm::vec2{x, pos.y + m_margins.top},
                            glm::vec2{x, pos.y + m_margins.top + m_plotImage.height()}, 1.0f, false, depth + 1);
        painter->strokeLine(glm::vec2{pos.x + m_margins.left, y},
                            glm::vec2{pos.x + m_margins.left + m_plotImage.width(), y}, 1.0f, false, depth + 1);
    }
}

bool MissionPlotGizmo::handleMousePress(const glm::vec2 &pos)
{
    updateMissionPlan(pos - glm::vec2{m_margins.left, m_margins.top});
    return true;
}

void MissionPlotGizmo::handleMouseRelease(const glm::vec2 &pos) {}

void MissionPlotGizmo::handleMouseMove(const glm::vec2 &pos)
{
    updateMissionPlan(pos - glm::vec2{m_margins.left, m_margins.top});
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
