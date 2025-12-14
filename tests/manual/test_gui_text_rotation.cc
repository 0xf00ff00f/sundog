#include <base/system.h>
#include <base/shader_manager.h>
#include <base/window_base.h>
#include <base/gui.h>
#include <base/painter.h>

#include <GLFW/glfw3.h>

#include <glm/gtx/string_cast.hpp>

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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    m_painter->begin();
    m_painter->setColor(glm::vec4{1.0f});

    const auto center = 0.5f * glm::vec2{m_viewportSize.width(), m_viewportSize.height()};
    m_painter->setFont(Font{"DejaVuSans.ttf", 60.0f, 0});

    m_painter->drawText(center, std::string_view{"0 degrees"}, Painter::Rotation::Rotate0);
    m_painter->drawText(center, std::string_view{"90 degrees"}, Painter::Rotation::Rotate90);
    m_painter->drawText(center, std::string_view{"180 degrees"}, Painter::Rotation::Rotate180);
    m_painter->drawText(center, std::string_view{"270 degrees"}, Painter::Rotation::Rotate270);

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
