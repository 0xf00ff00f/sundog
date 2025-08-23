#include "glhelpers.h"
#include "mesh.h"
#include "glyph_cache.h"
#include "tile_batcher.h"
#include "orbital_elements.h"
#include "universe.h"
#include "asset_path.h"
#include "shader_manager.h"
#include "lambert.h"

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/string_cast.hpp>

#include <nlohmann/json.hpp>

#include <chrono>
#include <fstream>
#include <cassert>
#include <cstdio>

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

    GLFWwindow *window = glfwCreateWindow(600, 600, "Hello", nullptr, nullptr);
    if (!window)
    {
        std::println(stderr, "Failed to create window");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    static bool playing = false;
    glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
            playing = !playing;
    });

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

        ShaderManager shaderManager;
        if (!shaderManager.initialize())
        {
            glfwTerminate();
            return 1;
        }

        std::ifstream f(dataFilePath("universe.json"));
        const nlohmann::json universeJson = nlohmann::json::parse(f);

        Universe universe;
        universe.load(universeJson);

        const auto transitInterval = JulianClock::duration{253.5};
        const auto timeDeparture = JulianDate{JulianClock::duration{2455892.126389}};
        const auto timeArrival = timeDeparture + transitInterval;
        Body spaceship;
        {
            const auto &worlds = universe.worlds();

            const auto &bodyDeparture = worlds[2].body(); // Earth
            const auto &bodyArrival = worlds[3].body();   // Mars

            auto posDeparture = glm::dvec3(bodyDeparture.position(timeDeparture));
            auto posArrival = glm::dvec3(bodyArrival.position(timeArrival));

            std::println("posDeparture={}", glm::to_string(posDeparture));
            std::println("posArrival={}", glm::to_string(posArrival));
            std::println("mu={}", kGMSun);
            std::println("transit={}", transitInterval.count());

            auto [velDeparture, velArrival] =
                *lambert_battin(kGMSun, posDeparture, posArrival, transitInterval.count());
            std::println("velDeparture={} velArrival={}", glm::to_string(velDeparture), glm::to_string(velArrival));

            const auto orbitalElements = orbitalElementsFromStateVector(posArrival, velArrival, timeArrival);
            spaceship.setOrbitalElements(orbitalElements);
        }

        auto modelMatrix = glm::mat4(1.0f);

        double lastCursorX = 0.0, lastCursorY = 0.0;
        glfwGetCursorPos(window, &lastCursorX, &lastCursorY);

        constexpr auto kCircleMeshVertexCount = 20;
        Mesh circleMesh;
        {
            constexpr auto kRadius = 0.05;

            // clang-format off
            const std::vector<glm::vec2> verts = std::views::iota(0, kCircleMeshVertexCount)
                                                 | std::views::transform([](const std::size_t i) -> glm::vec2 {
                                                       const auto a = i * 2.0f * glm::pi<float>() / kCircleMeshVertexCount;
                                                       const auto x = kRadius * glm::cos(a);
                                                       const auto y = kRadius * glm::sin(a);
                                                       return {x, y};
                                                   })
                                                 | std::ranges::to<std::vector>();
            // clang-format on
            circleMesh.setVertexData(std::as_bytes(std::span{verts}));
            const std::array<Mesh::VertexAttribute, 1> attributes = {Mesh::VertexAttribute{2, Mesh::Type::Float, 0}};
            circleMesh.setVertexAttributes(attributes, sizeof(glm::vec2));
        }

        GlyphCache glyphCache(fontFilePath("DejaVuSans.ttf"), 20);
        TileBatcher tileBatcher;

        const auto renderText = [&](const glm::vec2 &position, std::string_view text) {
            glm::vec2 p = position;
            for (size_t index = 0; const char ch : text)
            {
                const auto glyph = glyphCache.getGlyph(std::toupper(ch));
                if (glyph.has_value())
                {
                    tileBatcher.setTexture(glyph->texture);
                    tileBatcher.addTile({p + glyph->quad.topLeft, glyph->texCoords.topLeft},
                                        {p + glyph->quad.bottomRight, glyph->texCoords.bottomRight});
                    p += glm::vec2(glyph->advance, 0);
                    if (index < text.size() - 1)
                    {
                        p += glm::vec2(glyphCache.kernAdvance(ch, text[index + 1]), 0);
                    }
                }
                ++index;
            }
        };

        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        auto currentTime = timeDeparture; // JulianClock::now();

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            // TODO: proper arcball camera
            double cursorX = 0.0, cursorY = 0.0;
            glfwGetCursorPos(window, &cursorX, &cursorY);
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
            {
                constexpr auto RotationSpeed = 0.025;
                if (cursorX != lastCursorX)
                {
                    const auto dx = static_cast<float>(RotationSpeed * (cursorX - lastCursorX));
                    modelMatrix = glm::rotate(glm::mat4(1.0), dx, glm::vec3(0.0, 1.0, 0.0)) * modelMatrix;
                }
                if (cursorY != lastCursorY)
                {
                    const auto dy = static_cast<float>(RotationSpeed * (cursorY - lastCursorY));
                    modelMatrix = glm::rotate(glm::mat4(1.0), dy, glm::vec3(1.0, 0.0, 0.0)) * modelMatrix;
                }
            }
            lastCursorX = cursorX;
            lastCursorY = cursorY;

            int width, height;
            glfwGetWindowSize(window, &width, &height);

            glViewport(0, 0, width, height);
            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            const auto projectionMatrix =
                glm::perspective(glm::radians(45.0f), static_cast<float>(width) / height, 0.1f, 100.0f);
            const auto viewMatrix =
                glm::lookAt(glm::vec3(0.0f, 0.0f, 6.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            const auto mvp = projectionMatrix * viewMatrix * modelMatrix;

            shaderManager.setCurrent(ShaderManager::Shader::Wireframe);
            shaderManager.setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, mvp);
            shaderManager.setUniform(ShaderManager::Uniform::Color, glm::vec4(0.5, 0.5, 0.5, 1.0));

            const auto worlds = universe.worlds();

            for (const auto &world : worlds)
            {
                world.body().renderOrbit();
            }
            if (currentTime < timeArrival)
            {
                shaderManager.setUniform(ShaderManager::Uniform::Color, glm::vec4(0.5, 0.5, 0.0, 1.0));
                spaceship.renderOrbit();
            }

            shaderManager.setCurrent(ShaderManager::Shader::Billboard);
            shaderManager.setUniform(ShaderManager::Uniform::ProjectionMatrix, projectionMatrix);
            shaderManager.setUniform(ShaderManager::Uniform::ViewMatrix, viewMatrix);

            // sun
            shaderManager.setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, mvp);
            shaderManager.setUniform(ShaderManager::Uniform::ModelMatrix, modelMatrix);
            shaderManager.setUniform(ShaderManager::Uniform::Color, glm::vec4(0.5, 0.5, 0.5, 1.0));

            circleMesh.draw(Mesh::Primitive::LineLoop, 0, kCircleMeshVertexCount);

            const auto drawBody = [&](const Body &body, std::string_view name) {
                const auto position = body.position(currentTime);
                const auto bodyModelMatrix = modelMatrix * glm::translate(glm::mat4(1.0), position);
                const auto mvp = projectionMatrix * viewMatrix * bodyModelMatrix;
                shaderManager.setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, mvp);
                shaderManager.setUniform(ShaderManager::Uniform::ModelMatrix, bodyModelMatrix);
                circleMesh.draw(Mesh::Primitive::LineLoop, 0, kCircleMeshVertexCount);

                {
                    auto positionProjected = mvp * glm::vec4(0.0, 0.0, 0.0, 1.0);
                    glm::vec2 p;
                    p.x = 0.5f * ((positionProjected.x / positionProjected.w) + 1.0) * width;
                    p.y = (1.0f - 0.5f * ((positionProjected.y / positionProjected.w) + 1.0)) * height;

                    p.x += 5.0;
                    p.y -= glyphCache.pixelHeight();

                    renderText(p, name);
                }
            };

            tileBatcher.reset();
            for (const auto &world : worlds)
            {
                drawBody(world.body(), world.name());
            }
            if (currentTime < timeArrival)
            {
                shaderManager.setUniform(ShaderManager::Uniform::Color, glm::vec4(0.5, 0.5, 0.0, 1.0));
                drawBody(spaceship, "X");
            }

            renderText(glm::vec2(0), std::format("DATE={}", currentTime.time_since_epoch().count()));

            {
                shaderManager.setCurrent(ShaderManager::Shader::Text);
                const auto mvp = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f);
                shaderManager.setUniform(ShaderManager::Uniform::ModelViewProjectionMatrix, mvp);
                tileBatcher.blit();
            }

            glfwSwapBuffers(window);

            if (playing)
                currentTime += JulianClock::duration{0.25};
        }
    }

    glfwTerminate();
}
