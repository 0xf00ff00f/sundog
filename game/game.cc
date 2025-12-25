#include "game.h"

#include "universe.h"
#include "universe_map.h"
#include "mission_table.h"

#include "date_gizmo.h"
#include "trading_window.h"
#include "mission_plan_gizmo.h"

#include <base/asset_path.h>
#include <base/shader_manager.h>
#include <base/painter.h>

#include <glm/gtx/string_cast.hpp>

#include <mdspan>

namespace
{

std::optional<MissionPlan> findMissionPlan(const MissionTable *missionTable)
{
    std::optional<MissionPlan> bestPlan;

    std::println("{}x{}", missionTable->arrivals.size(), missionTable->departures.size());

    auto orbits = std::mdspan(missionTable->transferOrbits.data(), missionTable->arrivals.size(),
                              missionTable->departures.size());
    for (std::size_t i = 0; i != orbits.extent(0); ++i)
    {
        assert(i < missionTable->arrivals.size());
        const auto [timeArrival, posArrival, velWorldArrival] = missionTable->arrivals[i];
        for (std::size_t j = 0; j < orbits.extent(1); ++j)
        {
            assert(j < missionTable->departures.size());
            const auto [timeDeparture, posDeparture, velWorldDeparture] = missionTable->departures[j];
            if (const auto &orbit = orbits[i, j])
            {
                if (!bestPlan ||
                    orbit->deltaVDeparture + orbit->deltaVArrival < bestPlan->deltaVDeparture + bestPlan->deltaVArrival)
                {
                    const auto orbitalElements =
                        orbitalElementsFromStateVector(posArrival, orbit->velArrival, timeArrival);
                    bestPlan = MissionPlan{.origin = missionTable->origin(),
                                           .destination = missionTable->destination(),
                                           .departureDate = timeDeparture,
                                           .arrivalDate = timeArrival};
                    bestPlan->orbit.setElements(orbitalElements);
                    bestPlan->deltaVDeparture = orbit->deltaVDeparture;
                    bestPlan->deltaVArrival = orbit->deltaVArrival;
                }
            }
        }
    }

    return bestPlan;
}

} // namespace

Game::Game() = default;

Game::~Game() = default;

bool Game::initialize()
{
    m_universe = std::make_unique<Universe>();
    if (!m_universe->load(dataFilePath("universe.json")))
        return false;

    m_universe->setDate(JulianClock::now() + JulianYears{150.0});

    m_overlayPainter = std::make_unique<Painter>();

    m_universeMap = std::make_unique<UniverseMap>(m_universe.get(), m_overlayPainter.get());

    const auto &shipClasses = m_universe->shipClasses();
    const auto &worlds = m_universe->worlds();

    const auto *origin = worlds[2];      // Earth
    const auto *destination = worlds[11]; // Mars
    // const auto *destination = worlds[3]; // Mars
    const auto *shipClass = shipClasses[0];

    auto ship = m_universe->addShip(shipClass, origin, "SIGBUS");
    m_missionTable = std::make_unique<MissionTable>(origin, destination, m_universe->date(), 0.03);
    auto plan = findMissionPlan(m_missionTable.get());
    if (plan.has_value())
    {
        const auto transferDeparture = plan->orbit.stateVector(plan->departureDate);
        const auto transferArrival = plan->orbit.stateVector(plan->arrivalDate);
        std::println("departure: {} {}", glm::to_string(origin->orbit().position(plan->departureDate)),
                     glm::to_string(transferDeparture.position));
        std::println("arrival: {} {}", glm::to_string(destination->orbit().position(plan->arrivalDate)),
                     glm::to_string(transferArrival.position));
        const auto deltaV = plan->deltaVDeparture + plan->deltaVArrival;
        std::println("interval={} deltaV={} AU/days={} km/s", plan->transitTime().count(), deltaV,
                     deltaV * 1.496e+8 / (24 * 60 * 60));
        ship->setMissionPlan(std::move(plan.value()));
    }

    m_uiRoot = std::make_unique<ui::Rectangle>(100, 100);

    m_dateGizmo = m_uiRoot->appendChild<DateGizmo>(m_universe.get());
    m_dateGizmo->setAlign(ui::Align::Right | ui::Align::Top);

#if 0
    m_tradingWindow = m_uiRoot->appendChild<TradingWindow>(origin, ship);
    m_tradingWindow->setAlign(ui::Align::HorizontalCenter | ui::Align::VerticalCenter);
#endif
#if 0
    m_missionPlanGizmo = m_uiRoot->appendChild<MissionPlanGizmo>(ship, m_missionTable.get());
    m_missionPlanGizmo->setAlign(ui::Align::Left | ui::Align::VerticalCenter);

    m_missionPlanGizmo->confirmClickedSignal.connect([this] {
        m_uiRoot->removeChild(m_missionPlanGizmo);
        m_timeStep = JulianDate::duration{30.0f};
    });
#else
    if (auto missionPlan = ship->missionPlan())
        m_universe->setDate(missionPlan->departureDate + JulianDays{100.0});
    m_timeStep = JulianDays{std::chrono::seconds{1}};
#endif

    m_uiEventManager = std::make_unique<ui::EventManager>();
    m_uiEventManager->setRoot(m_uiRoot.get());

    glDisable(GL_CULL_FACE);

    return true;
}

void Game::setViewportSize(const SizeI &size)
{
    m_viewportSize = size;

    m_universeMap->setViewportSize(size);
    m_overlayPainter->setViewportSize(size);
    m_uiRoot->setSize(SizeF{size});
}

void Game::render() const
{
    glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());
    glClearColor(0.1, 0.12, 0.15, 1.0);
    glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_overlayPainter->begin();

    m_universeMap->render();

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_uiRoot->paint(m_overlayPainter.get(), glm::vec2{0.0f, 0.0f}, 0);

    m_overlayPainter->end();
}

void Game::update(Seconds elapsed)
{
    m_universe->update(elapsed.count() * m_timeStep);
    m_universeMap->update(elapsed);
}

void Game::handleKey(int /* key */, int /* scancode */, KeyAction /* action */, Modifier /* mods */) {}

void Game::handleMouseButton(MouseButton button, MouseAction action, const glm::vec2 &pos, Modifier mods)
{
    if (!m_uiEventManager->handleMouseButton(button, action, pos, mods))
        m_universeMap->handleMouseButton(button, action, pos, mods);
}

void Game::handleMouseMove(const glm::vec2 &pos)
{
    if (!m_uiEventManager->handleMouseMove(pos))
        m_universeMap->handleMouseMove(pos);
}

void Game::handleMouseWheel(const glm::vec2 &mousePos, const glm::vec2 &wheelOffset)
{
    if (!m_uiEventManager->handleMouseWheel(mousePos, wheelOffset))
        m_universeMap->handleMouseWheel(mousePos, wheelOffset);
}
