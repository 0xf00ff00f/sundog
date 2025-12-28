#pragma once

#include "julian_clock.h"

#include <base/window_base.h>

#include <glm/glm.hpp>

class Painter;
class ShaderManager;
class Universe;
class UniverseMap;
class DateGizmo;
class TradingWindow;
class MissionTable;
class MissionPlanGizmo;
class WorldInfoGizmo;
class ShipInfoGizmo;
class World;
class Ship;

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
    void handleMapSelection(World *world);
    void handleMapSelection(Ship *ship);
    void handleMapSelection(std::monostate);

    bool m_playing = false;
    SizeI m_viewportSize;
    std::unique_ptr<Universe> m_universe;
    std::unique_ptr<Painter> m_overlayPainter;
    std::unique_ptr<UniverseMap> m_universeMap;
    std::unique_ptr<ui::Rectangle> m_uiRoot;
    std::unique_ptr<ui::EventManager> m_uiEventManager;
    DateGizmo *m_dateGizmo{nullptr};
    TradingWindow *m_tradingWindow{nullptr};
    std::unique_ptr<MissionTable> m_missionTable;
    MissionPlanGizmo *m_missionPlanGizmo{nullptr};
    WorldInfoGizmo *m_worldInfoGizmo{nullptr};
    ShipInfoGizmo *m_shipInfoGizmo{nullptr};
    JulianDays m_timeStep{0.0}; // days/second
};
