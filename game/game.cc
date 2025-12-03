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

Game::Game() = default;

Game::~Game() = default;

bool Game::initialize()
{
    m_universe = std::make_unique<Universe>();
    if (!m_universe->load(dataFilePath("universe.json")))
        return false;

    m_shaderManager = std::make_unique<ShaderManager>();
    if (!m_shaderManager->initialize())
        return false;

    m_overlayPainter = std::make_unique<Painter>(m_shaderManager.get());

    m_universeMap = std::make_unique<UniverseMap>(m_universe.get(), m_shaderManager.get(), m_overlayPainter.get());

    const auto &worlds = m_universe->worlds();
    const auto *origin = worlds[2];      // Earth
    const auto *destination = worlds[3]; // Mars

    auto ship = m_universe->addShip(origin, "Mary Celeste");

    {
#if 0
        const auto transitInterval = JulianClock::duration{253.5};
        const auto timeDeparture = JulianDate{JulianClock::duration{2455892.126389}};
        const auto timeArrival = timeDeparture + transitInterval;

        auto posDeparture = glm::dvec3(origin->position(timeDeparture));
        auto posArrival = glm::dvec3(destination->position(timeArrival));

        std::println("posDeparture={}", glm::to_string(posDeparture));
        std::println("posArrival={}", glm::to_string(posArrival));
        std::println("mu={}", kGMSun);
        std::println("transit={}", transitInterval.count());

        auto result = lambert_battin(kGMSun, posDeparture, posArrival, transitInterval.count());
        assert(result.has_value());
        const auto [velDeparture, velArrival] = *result;

        const auto orbitalElements = orbitalElementsFromStateVector(posArrival, velArrival, timeArrival);

        MissionPlan transit{
            .origin = origin, .destination = destination, .departureTime = timeDeparture, .arrivalTime = timeArrival};
        transit.orbit.setElements(orbitalElements);

        ship->setMissionPlan(std::move(transit));

        m_universe->setDate(timeDeparture);
#else
        m_universe->setDate(JulianClock::now() + JulianClock::duration{200.0 * 365.0});
#endif
    }

    m_uiRoot = std::make_unique<ui::Rectangle>(100, 100);

    m_dateGizmo = m_uiRoot->appendChild<DateGizmo>(m_universe.get());
    m_dateGizmo->setAlign(ui::Align::Right | ui::Align::Top);

    m_tradingWindow = m_uiRoot->appendChild<TradingWindow>(origin, ship);
    m_tradingWindow->setAlign(ui::Align::HorizontalCenter | ui::Align::VerticalCenter);

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
    m_universe->update(elapsed);
    // m_universe->update(elapsed.count() * JulianClock::duration{20.0f});
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
    m_uiEventManager->handleMouseWheel(mousePos, wheelOffset);
}
