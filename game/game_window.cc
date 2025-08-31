#include "game_window.h"

#include "game.h"

GameWindow::GameWindow() = default;
GameWindow::~GameWindow() = default;

bool GameWindow::initializeResources()
{
    m_game = std::make_unique<Game>();
    if (!m_game->initialize())
        return false;

    m_game->setViewportSize(size());

    return true;
}

void GameWindow::handleWindowSize(const SizeI &size)
{
    m_game->setViewportSize(size);
}

void GameWindow::handleKey(int key, int scancode, KeyAction action, Modifier mods)
{
    m_game->handleKey(key, scancode, action, mods);
}

void GameWindow::handleMouseButton(int button, MouseAction action, Modifier mods)
{
    m_game->handleMouseButton(button, action, mods);
}

void GameWindow::handleMouseMove(double x, double y)
{
    m_game->handleMouseMove(x, y);
}

void GameWindow::update(Seconds elapsed)
{
    m_game->update(elapsed);
}

void GameWindow::render() const
{
    m_game->render();
}
