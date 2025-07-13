#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <cassert>
#include <stdio.h>

import glhelpers;
import std;

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

    GLFWwindow *window = glfwCreateWindow(400, 400, "Hello", nullptr, nullptr);
    if (!window)
    {
        std::println(stderr, "Failed to create window");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    const auto gladVersion = gladLoadGL(glfwGetProcAddress);
    if (gladVersion == 0)
    {
        std::println(stderr, "Failed to initialize OpenGL context");
        glfwTerminate();
        return 1;
    }
    std::println("Loaded OpenGL {}.{}", GLAD_VERSION_MAJOR(gladVersion), GLAD_VERSION_MINOR(gladVersion));
    std::println("OpenGL renderer: {}", reinterpret_cast<const char *>(glGetString(GL_RENDERER)));
    std::println("OpenGL version: {}", reinterpret_cast<const char *>(glGetString(GL_VERSION)));

    {
        using namespace gl;

        std::vector<float> points = {0.0f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f};
        Buffer buffer(Buffer::Target::ArrayBuffer);
        buffer.bind();
        buffer.data(std::as_bytes(std::span{points}), Buffer::Usage::StaticDraw);

        VertexArray vao;
        vao.bind();
        glEnableVertexAttribArray(0);
        buffer.bind();
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        const char *vertexShader = R"(
in vec3 position;
void main() {
    gl_Position = vec4(position, 1.0);
}
)";
        const char *fragmentShader = R"(
out vec4 fragColor;
void main() {
    fragColor = vec4(1.0);
}
)";
        ShaderProgram program;
        if (!program.attachShader(ShaderProgram::ShaderType::FragmentShader, fragmentShader))
        {
            glfwTerminate();
            return 1;
        }
        if (!program.attachShader(ShaderProgram::ShaderType::VertexShader, vertexShader))
        {
            glfwTerminate();
            return 1;
        }
        if (!program.link())
        {
            glfwTerminate();
            return 1;
        }

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            glClearColor(1.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            program.use();
            vao.bind();

            glDrawArrays(GL_TRIANGLES, 0, 3);

            glfwSwapBuffers(window);
        }
    }

    glfwTerminate();
}
