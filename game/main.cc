#include "game_window.h"

#include <base/system.h>

#include <GLFW/glfw3.h>

#include <cassert>
#include <print>

int main(int argc, char *argv[])
{
    System system;

    GameWindow window;
    if (window.initialize(1200, 800, "hello"))
    {
        window.run();
    }
}
