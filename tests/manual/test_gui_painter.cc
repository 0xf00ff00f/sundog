#include <base/shader_manager.h>
#include <base/window_base.h>
#include <base/gui.h>
#include <base/painter.h>

#include <GLFW/glfw3.h>

#include <glm/gtx/string_cast.hpp>

#include <print>

using namespace ui;

class TestWindow : public WindowBase
{
public:
    TestWindow();

    void handleWindowSize(const SizeI &size) override;

private:
    bool initializeResources() override;
    void update(Seconds elapsed) override;
    void render() const override;

    SizeI m_viewportSize;
    std::unique_ptr<ShaderManager> m_shaderManager;
    std::unique_ptr<Painter> m_painter;
    Seconds m_time{};
};

TestWindow::TestWindow() = default;

void TestWindow::handleWindowSize(const SizeI &size)
{
    m_viewportSize = size;
    m_painter->setViewportSize(size);
}

bool TestWindow::initializeResources()
{
    m_shaderManager = std::make_unique<ShaderManager>();
    if (!m_shaderManager->initialize())
        return false;

    m_painter = std::make_unique<Painter>(m_shaderManager.get());

    return true;
}

void TestWindow::update(Seconds elapsed)
{
    m_time += elapsed;
}

void TestWindow::render() const
{
    glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);

    m_painter->begin();
    m_painter->setColor(glm::vec4{1.0f});

    {
        constexpr auto kVertexCount = 10;
        constexpr auto kSmallRadius = 100.0f;
        constexpr auto kBigRadius = 200.0f;
        constexpr auto kThickness = 20.0f;

        const auto center = 0.5f * glm::vec2{m_viewportSize.width(), m_viewportSize.height()};

        std::array<glm::vec2, kVertexCount> verts;
        for (std::size_t i = 0; i < kVertexCount; ++i)
        {
            const auto angle = m_time.count() * 0.1f + 2.0f * glm::pi<float>() * i / kVertexCount;
            const auto radius = (i & 1) ? kSmallRadius : kBigRadius;
            verts[i] = center + radius * glm::vec2{glm::cos(angle), glm::sin(angle)};
        }
        m_painter->drawPolyline(verts, kThickness, true);
    }

    m_painter->end();
}

int main(int argc, char *argv[])
{
    if (!glfwInit())
    {
        std::println(stderr, "Failed to initialized GLFW");
        return 1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    {
        TestWindow w;
        if (w.initialize(600, 600, "test"))
        {
            w.run();
        }
    }

    glfwTerminate();
}
