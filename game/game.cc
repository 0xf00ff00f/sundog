#include "game.h"

#include "asset_path.h"
#include "universe.h"
#include "shader_manager.h"
#include "painter.h"
#include "universe_map.h"
#include "lambert.h"

#include <fstream>

#include <nlohmann/json.hpp>

#include <glm/gtx/string_cast.hpp>

Game::Game() = default;

Game::~Game() = default;

bool Game::initialize()
{
    std::ifstream f(dataFilePath("universe.json"));
    const nlohmann::json universeJson = nlohmann::json::parse(f);

    m_universe = std::make_unique<Universe>();
    if (!m_universe->load(universeJson))
        return false;

    m_shaderManager = std::make_unique<ShaderManager>();
    if (!m_shaderManager->initialize())
        return false;

    m_overlayPainter = std::make_unique<Painter>(m_shaderManager.get());

    m_universeMap = std::make_unique<UniverseMap>(m_universe.get(), m_shaderManager.get(), m_overlayPainter.get());

    auto ship = m_universe->addShip("Foo");
    {
        const auto transitInterval = JulianClock::duration{253.5};
        const auto timeDeparture = JulianDate{JulianClock::duration{2455892.126389}};
        const auto timeArrival = timeDeparture + transitInterval;

        const auto &worlds = m_universe->worlds();
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
}

void Game::render() const
{
    glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_overlayPainter->begin();
    m_universeMap->render(m_currentTime);
    m_overlayPainter->drawText(glm::vec2(0), std::format("DATE={}", m_currentTime.time_since_epoch().count()));
    m_overlayPainter->end();
}

void Game::update(Seconds elapsed)
{
    m_currentTime += elapsed.count() * JulianClock::duration{20.0};
}

void Game::handleKey(int /* key */, int /* scancode */, KeyAction /* action */, Modifier /* mods */) {}

void Game::handleMouseButton(MouseButton button, MouseAction action, Modifier mods)
{
    m_universeMap->handleMouseButton(button, action, mods);
}

void Game::handleMouseMove(const glm::dvec2 &position)
{
    m_universeMap->handleMouseMove(position);
}
