#pragma once

#include <base/window_base.h>

#include <glm/glm.hpp>

class Painter;
class ShaderManager;
class Universe;
class UniverseMap;
class DateGizmo;
class TradingWindow;

namespace ui
{
class Rectangle;
class EventManager;
}; // namespace ui

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
    void handleMouseWheel(const glm::vec2 &mousePos, const glm::vec2 &wheelOffset);

private:
    bool m_playing = false;
    SizeI m_viewportSize;
    std::unique_ptr<Universe> m_universe;
    std::unique_ptr<Painter> m_overlayPainter;
    std::unique_ptr<UniverseMap> m_universeMap;
    std::unique_ptr<ui::Rectangle> m_uiRoot;
    std::unique_ptr<ui::EventManager> m_uiEventManager;
    DateGizmo *m_dateGizmo{nullptr};
    TradingWindow *m_tradingWindow{nullptr};
};
