#include "window_base.h"

#include <print>

WindowBase::WindowBase() = default;

WindowBase::~WindowBase()
{
    if (m_window)
        glfwDestroyWindow(m_window);
}

bool WindowBase::initialize(int width, int height, const char *title)
{
    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_window)
        return false;

    glfwMakeContextCurrent(m_window);
    const auto gladVersion = gladLoadGL(glfwGetProcAddress);
    if (gladVersion == 0)
        return false;

    std::println("Loaded OpenGL {}.{}", GLAD_VERSION_MAJOR(gladVersion), GLAD_VERSION_MINOR(gladVersion));
    std::println("OpenGL renderer: {}", reinterpret_cast<const char *>(glGetString(GL_RENDERER)));
    std::println("OpenGL version: {}", reinterpret_cast<const char *>(glGetString(GL_VERSION)));

    glfwSetWindowUserPointer(m_window, this);
    glfwSetWindowSizeCallback(m_window, WindowBase::handleWindowSize);
    glfwSetKeyCallback(m_window, WindowBase::handleKey);
    glfwSetMouseButtonCallback(m_window, WindowBase::handleMouseButton);
    glfwSetCursorPosCallback(m_window, WindowBase::handleMouseMove);

    initializeResources();

    return true;
}

void WindowBase::handleWindowSize(GLFWwindow *window, int width, int height)
{
    auto *self = static_cast<WindowBase *>(glfwGetWindowUserPointer(window));
    self->handleWindowSize(width, height);
}

void WindowBase::handleKey(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    auto *self = static_cast<WindowBase *>(glfwGetWindowUserPointer(window));
    self->handleKey(key, scancode, static_cast<KeyAction>(action), static_cast<Modifier>(mods));
}

void WindowBase::handleMouseButton(GLFWwindow *window, int button, int action, int mods)
{
    auto *self = static_cast<WindowBase *>(glfwGetWindowUserPointer(window));
    self->handleMouseButton(button, static_cast<MouseAction>(action), static_cast<Modifier>(mods));
}

void WindowBase::handleMouseMove(GLFWwindow *window, double x, double y)
{
    auto *self = static_cast<WindowBase *>(glfwGetWindowUserPointer(window));
    self->handleMouseMove(x, y);
}

void WindowBase::handleWindowSize(int /* width */, int /* height */)
{
}

void WindowBase::handleKey(int /* key */, int /* scancode */, KeyAction /* action */, Modifier /* mods */)
{
}

void WindowBase::handleMouseButton(int /* button */, MouseAction /* action */, Modifier /* mods */)
{
}

void WindowBase::handleMouseMove(double /* x */, double /* y */)
{
}

void WindowBase::run()
{
    auto tPrev = Seconds{ 0.0 };
    glfwSetTime(0.0);

    for (;;)
    {
        const auto t = Seconds{ glfwGetTime() };
        const auto elapsed = t - tPrev;
        tPrev = t;

        update(elapsed);
        render();

        glfwSwapBuffers(m_window);
        glfwPollEvents();

        if (glfwWindowShouldClose(m_window))
            break;
    }
}

SizeI WindowBase::size() const
{
    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    return SizeI(width, height);
}

glm::dvec2 WindowBase::cursorPos() const
{
    double x, y;
    glfwGetCursorPos(m_window, &x, &y);
    return {x, y};
}
