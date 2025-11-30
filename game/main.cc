#include "game_window.h"

#include <GLFW/glfw3.h>

#include <cassert>
#include <print>

int main(int argc, char *argv[])
{
    if (!glfwInit())
    {
        std::println(stderr, "Failed to initialize GLFW");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    {
        GameWindow window;
        if (window.initialize(1200, 800, "hello"))
        {
            window.run();
        }
    }

    glfwTerminate();
}
