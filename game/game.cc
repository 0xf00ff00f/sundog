#include "game.h"

#include "universe.h"
#include "universe_map.h"
#include "lambert.h"

#include "date_gizmo.h"
#include "market_snapshot_gizmo.h"

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

#if 0
    auto ship = m_universe.addShip("Foo");
    {
        const auto transitInterval = JulianClock::duration{253.5};
        const auto timeDeparture = JulianDate{JulianClock::duration{2455892.126389}};
        const auto timeArrival = timeDeparture + transitInterval;

        const auto &worlds = m_universe.worlds();
        const auto *origin = worlds[2];      // Earth
        const auto *destination = worlds[3]; // Mars

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

        Transit transit{
            .origin = origin, .destination = destination, .departureTime = timeDeparture, .arrivalTime = timeArrival};
        transit.orbit.setElements(orbitalElements);

        ship->setTransit(std::move(transit));

        m_currentTime = timeDeparture;
    }
#else
    m_currentTime = JulianClock::now();
#endif

    m_uiRoot = std::make_unique<ui::Rectangle>(100, 100);

    m_dateGizmo = m_uiRoot->appendChild<DateGizmo>();
    m_dateGizmo->setDate(m_currentTime);
    m_dateGizmo->setAlign(ui::Align::Left | ui::Align::Top);

    m_marketSnapshotGizmo = m_uiRoot->appendChild<MarketSnapshotGizmo>();
    m_marketSnapshotGizmo->setAlign(ui::Align::HorizontalCenter | ui::Align::VerticalCenter);
    m_marketSnapshotGizmo->initializeFrom(m_universe->worlds().front());

    m_marketSnapshotGizmo->itemSelectedSignal.connect(
        [](const MarketItemDescription *item) { std::println("*** item selected: {}", item->name); });

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
    m_universeMap->render(m_currentTime);

    m_uiRoot->paint(m_overlayPainter.get(), glm::vec2{0.0f, 0.0f}, 0);

    m_overlayPainter->end();
}

void Game::update(Seconds elapsed)
{
#if 0
    m_currentTime += elapsed.count() * JulianClock::duration{20.0f};
#else
    m_currentTime += elapsed;
#endif
    m_dateGizmo->setDate(m_currentTime);
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
