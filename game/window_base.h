#pragma once

#include "rect.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <chrono>

enum class KeyAction
{
    Press = GLFW_PRESS,
    Release = GLFW_RELEASE,
    Repeat = GLFW_REPEAT
};

enum class Modifier
{
    Shift = GLFW_MOD_SHIFT,
    Control = GLFW_MOD_CONTROL,
    Alt = GLFW_MOD_ALT,
    Super = GLFW_MOD_SUPER,
    CapsLock = GLFW_MOD_CAPS_LOCK,
    NumLock = GLFW_MOD_NUM_LOCK
};

enum class MouseAction
{
    Press = GLFW_PRESS,
    Release = GLFW_RELEASE
};

using Seconds = std::chrono::duration<double, std::ratio<1>>;

class WindowBase
{
public:
    WindowBase();
    virtual ~WindowBase();

    WindowBase(WindowBase &&) = delete;
    WindowBase &operator=(WindowBase &&) = delete;

    WindowBase(const WindowBase &) = delete;
    WindowBase &operator=(const WindowBase &) = delete;

    bool initialize(int width, int height, const char *title);
    void run();

    SizeI size() const;
    glm::dvec2 cursorPos() const;

    static void handleWindowSize(GLFWwindow *window, int width, int height);
    static void handleKey(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void handleMouseButton(GLFWwindow *window, int button, int action, int mods);
    static void handleMouseMove(GLFWwindow *window, double x, double y);

    virtual void handleWindowSize(const SizeI &size);
    virtual void handleKey(int key, int scancode, KeyAction action, Modifier mods);
    virtual void handleMouseButton(int button, MouseAction action, Modifier mods);
    virtual void handleMouseMove(double x, double y);

protected:
    virtual bool initializeResources() = 0;
    virtual void update(Seconds elapsed) = 0;
    virtual void render() const = 0;

private:
    bool initializeGlad();

    GLFWwindow *m_window = nullptr;
};
