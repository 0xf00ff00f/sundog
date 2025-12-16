#include "mission_plan_gizmo.h"

#include "universe.h"
#include "mission_table.h"
#include "mission_plot_gizmo.h"
#include "style_settings.h"
#include "button_gizmo.h"

using namespace ui;

namespace
{

constexpr auto kTotalWidth = 400.0f;

void addSeparator(Gizmo *parent, float width, const glm::vec4 &color)
{
    auto *separator = parent->appendChild<Rectangle>(width, 1.0f);
    separator->setFillBackground(true);
    separator->backgroundColor = color;
}

constexpr double toKmS(double speed)
{
    return speed * 1.496e+8 / (24 * 60 * 60);
}

} // namespace

MissionPlanGizmo::MissionPlanGizmo(Ship *ship, const MissionTable *missionTable, Gizmo *parent)
    : ui::Column(parent)
    , m_ship(ship)
{
    const auto *origin = missionTable->origin();
    const auto *destination = missionTable->destination();

    setFillBackground(true);
    setMargins(20.0f);
    backgroundColor = glm::vec4{0.0f, 0.0f, 0.0f, 0.75f};

    auto addSectionHeader = [this](std::string_view text) {
        auto *label = appendChild<Text>();
        label->setFont(g_styleSettings.normalFont);
        label->color = g_styleSettings.accentColor;
        label->setText(text);

        addSeparator(this, kTotalWidth, g_styleSettings.baseColor);
    };

    auto addDataRow = [this](std::string_view titleText) {
        auto *row = appendChild<Row>();
        row->setSpacing(0.0f);

        auto *titleContainer = row->appendChild<Column>();
        titleContainer->setMinimumWidth(250.0f);
        auto *titleLabel = titleContainer->appendChild<Text>();
        titleLabel->setFont(g_styleSettings.normalFont);
        titleLabel->color = g_styleSettings.baseColor;
        titleLabel->setText(titleText);

        auto *dataText = row->appendChild<Text>();
        dataText->setFont(g_styleSettings.normalFont);
        dataText->color = g_styleSettings.accentColor;
        return dataText;
    };

    auto addSpacer = [this] { appendChild<Rectangle>(0.0f, 20.0f); };

    auto *title = appendChild<Text>();
    title->setFont(g_styleSettings.titleFont);
    title->color = g_styleSettings.accentColor;
    title->setText("Mission Plan");

    addSeparator(this, kTotalWidth, g_styleSettings.baseColor);

    addSpacer();

    auto *fromToRow = appendChild<Row>();
    fromToRow->setSpacing(20.0f);

    auto *fromColumn = fromToRow->appendChild<Column>();
    auto *fromLabel = fromColumn->appendChild<Text>(g_styleSettings.smallFont, "Origin");
    fromLabel->color = g_styleSettings.baseColor;
    addSeparator(fromColumn, (kTotalWidth - fromToRow->spacing()) / 2.0f, g_styleSettings.baseColor);
    auto *fromText = fromColumn->appendChild<Text>();
    fromText->setFont(g_styleSettings.normalFont);
    fromText->color = g_styleSettings.accentColor;
    fromText->setText(origin->name);

    auto *toColumn = fromToRow->appendChild<Column>();
    auto *toLabel = toColumn->appendChild<Text>(g_styleSettings.smallFont, "Destination");
    toLabel->color = g_styleSettings.baseColor;
    addSeparator(toColumn, (kTotalWidth - fromToRow->spacing()) / 2.0f, g_styleSettings.baseColor);
    auto *toText = toColumn->appendChild<Text>();
    toText->setFont(g_styleSettings.normalFont);
    toText->color = g_styleSettings.accentColor;
    toText->setText(destination->name);

    addSpacer();

    addSectionHeader("Total Delta-v Plot");
    m_missionPlotGizmo = appendChild<MissionPlotGizmo>(missionTable);
    m_missionPlotGizmo->setAlign(Align::HorizontalCenter | Align::VerticalCenter);
    addSpacer();

    addSectionHeader("Trajectory");
    m_departureDateText = addDataRow("Departure");
    m_arrivalDateText = addDataRow("Arrival");
    m_transitTimeText = addDataRow("Transit time");
    addSpacer();

    addSectionHeader("Delta-v Requirements");
    m_departureDeltaVText = addDataRow(std::format("Trans-{} Injection", destination->name));
    m_arrivalDeltaVText = addDataRow(std::format("{} Orbit Injection", destination->name));
    m_totalDeltaVText = addDataRow("Total");
    addSpacer();

    auto *confirmButton = appendChild<ButtonGizmo>("Confirm");
    confirmButton->setSize(80, 30);
    confirmButton->setAlign(Align::Right);

    m_missionPlotGizmo->setMissionPlan(m_ship->missionPlan());

    updateTrajectoryValues();

    m_missionPlotGizmo->missionPlanChangedSignal.connect([this]() {
        updateTrajectoryValues();
        m_ship->setMissionPlan(m_missionPlotGizmo->missionPlan());
    });

    confirmButton->clickedSignal.connect([this] { confirmClickedSignal(); });
}

void MissionPlanGizmo::updateTrajectoryValues()
{
    const auto missionPlan = m_missionPlotGizmo->missionPlan();

    if (missionPlan)
    {
        m_departureDateText->setText(std::format("{:D}", missionPlan->departureDate));
        m_arrivalDateText->setText(std::format("{:D}", missionPlan->arrivalDate));
        const auto transitTime = missionPlan->transitTime();
        if (transitTime > JulianYears{1})
            m_transitTimeText->setText(std::format("{:.2f} years", JulianYears{transitTime}.count()));
        else
            m_transitTimeText->setText(std::format("{:.2f} days", transitTime.count()));
        m_departureDeltaVText->setText(std::format("{:.2f} km/s", toKmS(missionPlan->deltaVDeparture)));
        m_arrivalDeltaVText->setText(std::format("{:.2f} km/s", toKmS(missionPlan->deltaVArrival)));
        m_totalDeltaVText->setText(
            std::format("{:.2f} km/s", toKmS(missionPlan->deltaVDeparture + missionPlan->deltaVArrival)));
    }
    else
    {
        m_departureDateText->setText("-");
        m_arrivalDateText->setText("-");
        m_transitTimeText->setText("-");
        m_departureDeltaVText->setText("-");
        m_arrivalDeltaVText->setText("-");
        m_totalDeltaVText->setText("-");
    }
}
