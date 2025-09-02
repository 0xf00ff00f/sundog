#pragma once

#include "window_base.h"

#include <memory>

class Game;

class GameWindow : public WindowBase
{
public:
    GameWindow();
    ~GameWindow() override;

    void handleWindowSize(const SizeI &size) override;
    void handleKey(int key, int scancode, KeyAction action, Modifier mods) override;
    void handleMouseButton(MouseButton button, MouseAction action, Modifier mods) override;
    void handleMouseMove(const glm::dvec2 &position) override;

private:
    bool initializeResources() override;
    void update(Seconds elapsed) override;
    void render() const override;

    std::unique_ptr<Game> m_game;
};
