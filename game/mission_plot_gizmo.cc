#include "mission_plot_gizmo.h"

#include "mission_plot.h"
#include "style_settings.h"

#include <glm/gtx/string_cast.hpp>

#include <cassert>
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

    if (m_missionPlan)
    {
        const auto arrivalIndex = [this] {
            const auto &arrivals = m_missionTable->arrivals;
            auto it =
                std::ranges::lower_bound(arrivals, m_missionPlan->arrivalDate, {}, &MissionTable::DateState::date);
            assert(it != arrivals.end());
            return std::distance(arrivals.begin(), it);
        }();
        const auto departureIndex = [this] {
            const auto &departures = m_missionTable->departures;
            auto it =
                std::ranges::lower_bound(departures, m_missionPlan->departureDate, {}, &MissionTable::DateState::date);
            assert(it != departures.end());
            return std::distance(departures.begin(), it);
        }();
        const auto left = pos.x + m_margins.left;
        const auto top = pos.y + m_margins.top;
        const auto x = left + departureIndex;
        const auto y = top + m_plotImage.height() - arrivalIndex;
        painter->setColor(glm::vec4{0.0f, 0.0f, 0.0f, 1.0f});
        painter->strokeLine(glm::vec2{x, top}, glm::vec2{x, top + m_plotImage.height()}, 1.0f, false, depth + 1);
        painter->strokeLine(glm::vec2{left, y}, glm::vec2{left + m_plotImage.width(), y}, 1.0f, false, depth + 1);
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

        auto transfers = std::mdspan(m_missionTable->transferOrbits.data(), m_missionTable->arrivals.size(),
                                     m_missionTable->departures.size());
        const auto &transfer = transfers[arrivalIndex, departureIndex];
        if (!transfer.has_value())
            return {};

        const auto [dateArrival, posArrival, velWorldArrival] = m_missionTable->arrivals[arrivalIndex];
        const auto [dateDeparture, posDeparture, velWorldDeparture] = m_missionTable->departures[departureIndex];

        const auto orbitalElements = orbitalElementsFromStateVector(posArrival, transfer->velArrival, dateArrival);
        const auto *origin = m_missionTable->origin();
        const auto *destination = m_missionTable->destination();

        MissionPlan missionPlan{
            .origin = origin, .destination = destination, .departureDate = dateDeparture, .arrivalDate = dateArrival};
        missionPlan.orbit.setElements(orbitalElements);
        missionPlan.deltaVDeparture = transfer->deltaVDeparture;
        missionPlan.deltaVArrival = transfer->deltaVArrival;

        {
            // TODO sanity check, remove this and add unit tests
            const auto [orbitPosDeparture, orbitVelDeparture] = missionPlan.orbit.stateVector(dateDeparture);
            const auto [orbitPosArrival, orbitVelArrival] = missionPlan.orbit.stateVector(dateArrival);
            const auto closeEnough = [](const glm::dvec3 &a, const glm::dvec3 &b) {
                constexpr auto kTolerance = 1e-6;
                return glm::distance(a, b) < kTolerance;
            };
            assert(closeEnough(orbitPosDeparture, posDeparture));
            assert(closeEnough(orbitVelDeparture, transfer->velDeparture));
            assert(closeEnough(orbitPosArrival, posArrival));
            assert(closeEnough(orbitVelArrival, transfer->velArrival));
        }

        return missionPlan;
    }();

    m_missionPlan = missionPlan;
    missionPlanChangedSignal();
}

void MissionPlotGizmo::setMissionPlan(std::optional<MissionPlan> missionPlan)
{
    m_missionPlan = std::move(missionPlan);
}
