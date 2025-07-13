#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <stdio.h>

import std;

int main(int argc, char *argv[])
{
    if (!glfwInit()) {
        std::println(stderr, "Failed to initialize GLFW");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(400, 400, "Hello", nullptr, nullptr);
    if (!window) {
        std::println(stderr, "Failed to create window");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    const auto gladVersion = gladLoadGL(glfwGetProcAddress);
    if (gladVersion == 0) {
        std::println(stderr, "Failed to initialize OpenGL context");
        glfwTerminate();
        return 1;
    }
    std::println("Loaded OpenGL {}.{}", GLAD_VERSION_MAJOR(gladVersion), GLAD_VERSION_MINOR(gladVersion));
    std::println("OpenGL renderer: {}", reinterpret_cast<const char *>(glGetString(GL_RENDERER)));
    std::println("OpenGL version: {}", reinterpret_cast<const char *>(glGetString(GL_VERSION)));

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // TODO

        glfwSwapBuffers(window);
    }

    glfwTerminate();
}
