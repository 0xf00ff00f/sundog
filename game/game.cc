#include "game.h"

#include "asset_path.h"
#include "universe.h"
#include "shader_manager.h"
#include "painter.h"
#include "universe_map.h"

#include <fstream>

#include <nlohmann/json.hpp>

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

    m_currentTime = JulianClock::now();

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

void Game::handleKey(int /* key */, int /* scancode */, KeyAction /* action */, Modifier /* mods */)
{
}

void Game::handleMouseButton(int /* button */, MouseAction /* action */, Modifier /* mods */)
{
}

void Game::handleMouseMove(double /* x */, double /* y */)
{
}
