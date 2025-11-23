#pragma once

#include "julian_clock.h"
#include "universe.h"

#include <base/window_base.h>

#include <glm/glm.hpp>

class ShaderManager;
class Painter;
class UniverseMap;
class DateGizmo;

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
    void handleMouseButton(MouseButton button, MouseAction action, const glm::vec2 &pos, Modifier mods);
    void handleMouseMove(const glm::vec2 &pos);

private:
    bool m_playing = false;
    SizeI m_viewportSize;
    Universe m_universe;
    std::unique_ptr<ShaderManager> m_shaderManager;
    std::unique_ptr<Painter> m_overlayPainter;
    std::unique_ptr<UniverseMap> m_universeMap;
    JulianDate m_currentTime;
    std::unique_ptr<DateGizmo> m_dateGizmo;
};
