#include "game.h"

#include "universe.h"
#include "universe_map.h"
#include "lambert.h"

#include "date_gizmo.h"
#include "trading_window.h"

#include <base/asset_path.h>
#include <base/shader_manager.h>
#include <base/painter.h>

#include <glm/gtx/string_cast.hpp>

namespace
{

std::optional<MissionPlan> findMissionPlan(const World *origin, const World *destination, JulianDate start)
{
    std::optional<MissionPlan> bestPlan;

    constexpr JulianClock::duration kMinTransitInterval{90.0};
    constexpr JulianClock::duration kMaxTransitInterval{4.0 * 365.0};
    constexpr JulianClock::duration kMaxWait{3.0 * 365.0};
    constexpr JulianClock::duration kStep{5.0};

    for (JulianDate timeDeparture = start; timeDeparture < start + kMaxWait; timeDeparture += kStep)
    {
        const auto posDeparture = origin->orbit().position(timeDeparture);
        const auto velWorldDeparture = origin->orbit().velocity(timeDeparture);

        for (JulianClock::duration transitInterval = kMinTransitInterval; transitInterval < kMaxTransitInterval;
             transitInterval += kStep)
        {
            const auto timeArrival = timeDeparture + transitInterval;
            const auto posArrival = destination->orbit().position(timeArrival);
            const auto velWorldArrival = destination->orbit().velocity(timeArrival);

            auto result = lambert_battin(kGMSun, posDeparture, posArrival, transitInterval.count());
            if (result.has_value())
            {
                const auto [velDeparture, velArrival] = *result;
                const auto orbitalElements = orbitalElementsFromStateVector(posArrival, velArrival, timeArrival);
                const auto deltaV = glm::length(glm::vec3{velDeparture} - origin->orbit().velocity(timeDeparture)) +
                                    glm::length(glm::vec3{velArrival} - destination->orbit().velocity(timeArrival));
                if (!bestPlan.has_value() || bestPlan->deltaV > deltaV)
                {
                    bestPlan = MissionPlan{.origin = origin,
                                           .destination = destination,
                                           .departureTime = timeDeparture,
                                           .arrivalTime = timeArrival};
                    bestPlan->orbit.setElements(orbitalElements);
                    bestPlan->deltaV = deltaV;
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

    m_universe->setDate(JulianClock::now());

    m_overlayPainter = std::make_unique<Painter>();

    m_universeMap = std::make_unique<UniverseMap>(m_universe.get(), m_overlayPainter.get());

    const auto &shipClasses = m_universe->shipClasses();
    const auto &worlds = m_universe->worlds();

    const auto *origin = worlds[2];      // Earth
    const auto *destination = worlds[8]; // Mars
    const auto *shipClass = shipClasses[1];

    auto ship = m_universe->addShip(shipClass, origin, "SIGBUS");
#if 1
    auto plan = findMissionPlan(origin, destination, JulianClock::now());
    if (plan.has_value())
    {
        std::println("interval={} deltaV={} AU/days={} km/s", plan->transitTime().count(), plan->deltaV,
                     plan->deltaV * 1.496e+8 / (24 * 60 * 60));
        m_universe->setDate(plan->departureTime);
        ship->setMissionPlan(std::move(plan.value()));
    }
#endif

    m_uiRoot = std::make_unique<ui::Rectangle>(100, 100);

    m_dateGizmo = m_uiRoot->appendChild<DateGizmo>(m_universe.get());
    m_dateGizmo->setAlign(ui::Align::Right | ui::Align::Top);

#if 0
    m_tradingWindow = m_uiRoot->appendChild<TradingWindow>(origin, ship);
    m_tradingWindow->setAlign(ui::Align::HorizontalCenter | ui::Align::VerticalCenter);
#endif

    m_uiEventManager = std::make_unique<ui::EventManager>();
    m_uiEventManager->setRoot(m_uiRoot.get());

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_overlayPainter->begin();
    m_universeMap->render();

    m_uiRoot->paint(m_overlayPainter.get(), glm::vec2{0.0f, 0.0f}, 0);

    m_overlayPainter->end();
}

void Game::update(Seconds elapsed)
{
    // m_universe->update(elapsed);
    m_universe->update(elapsed.count() * JulianClock::duration{30.0f});
    // m_universe->update(elapsed.count() * JulianClock::duration{10.0f});
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
