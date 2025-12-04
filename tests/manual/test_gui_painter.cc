#include <base/system.h>
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
    m_painter = std::make_unique<Painter>();
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
        constexpr auto kSmallRadius = 60.0f;
        constexpr auto kBigRadius = 130.0f;
        constexpr auto kThickness = 20.0f;

        const auto center = 0.25f * glm::vec2{m_viewportSize.width(), m_viewportSize.height()};

        std::array<glm::vec2, kVertexCount> verts;
        for (std::size_t i = 0; i < kVertexCount; ++i)
        {
            const auto angle = m_time.count() * 0.1f + 2.0f * glm::pi<float>() * i / kVertexCount;
            const auto radius = (i & 1) ? kSmallRadius : kBigRadius;
            verts[i] = center + radius * glm::vec2{glm::cos(angle), glm::sin(angle)};
        }
        m_painter->strokePolyline(verts, kThickness, true);
    }

    {
        const Painter::CornerRadii radii{30.0f, 50.0f, 70.0f, 90.0f};
        const auto center = glm::vec2{0.25f, 0.75f} * glm::vec2{m_viewportSize.width(), m_viewportSize.height()};
        const RectF rect{center - glm::vec2{120.f, 100.f}, center + glm::vec2{120.f, 100.f}};
        m_painter->fillRoundedRect(rect, radii);
    }

    {
        const Painter::CornerRadii radii{30.0f, 50.0f, 70.0f, 90.0f};
        const auto center = glm::vec2{0.75f, 0.75f} * glm::vec2{m_viewportSize.width(), m_viewportSize.height()};
        const RectF rect{center - glm::vec2{120.f, 100.f}, center + glm::vec2{120.f, 100.f}};
        m_painter->strokeRoundedRect(rect, radii, 10.0f);
    }

    {
        const auto pos = glm::vec2{0.75f, 0.25f} * glm::vec2{m_viewportSize.width(), m_viewportSize.height()} -
                         glm::vec2{64.0f, 64.0f};
        m_painter->drawIcon(pos, "vim.png");
    }

    m_painter->end();
}

int main(int argc, char *argv[])
{
    System system;

    TestWindow w;
    if (w.initialize(600, 600, "test"))
    {
        w.run();
    }
}
