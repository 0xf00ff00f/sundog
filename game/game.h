#pragma once

#include "window_base.h"
#include "julian_clock.h"

#include <glm/glm.hpp>

class Universe;
class ShaderManager;
class Painter;
class UniverseMap;

class Game
{
public:
    Game();
    ~Game();

    Game(Game &&) = delete;
    Game &operator=(Game &&) = delete;

    Game(const Game &) = delete;
    Game &operator=(const Game &) = delete;

    bool initialize();

    void setViewportSize(const SizeI &size);
    void render() const;
    void update(Seconds elapsed);

    void handleWindowSize(int x, int y);
    void handleKey(int key, int scancode, KeyAction action, Modifier mods);
    void handleMouseButton(int button, MouseAction action, Modifier mods);
    void handleMouseMove(double x, double y);

private:
    bool m_playing = false;
    SizeI m_viewportSize;
    std::unique_ptr<Universe> m_universe;
    std::unique_ptr<ShaderManager> m_shaderManager;
    std::unique_ptr<Painter> m_overlayPainter;
    std::unique_ptr<UniverseMap> m_universeMap;
    JulianDate m_currentTime;
};
