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
    void handleKey(int key, int scancode, KeyAction action, Modifier mods) override;
    void handleMouseButton(MouseButton button, MouseAction action, Modifier mods) override;
    void handleMouseMove(const glm::vec2 &pos) override;
    void handleMouseWheel(const glm::vec2 &offset) override;

private:
    bool initializeResources() override;
    void update(Seconds elapsed) override;
    void render() const override;

    SizeI m_viewportSize;
    std::unique_ptr<Painter> m_painter;
    std::unique_ptr<Gizmo> m_uiRoot;
    EventManager m_eventManager;
};

TestWindow::TestWindow()
{
    constexpr auto kSpacing = 8.0f;

    auto outerLayout = std::make_unique<Row>();
    outerLayout->setMargins(40.0f);
    outerLayout->backgroundColor = glm::vec4{1.0f};
    outerLayout->setFillBackground(true);

    auto scrollArea = outerLayout->appendChild<ScrollArea>(400.0f, 400.0f);
    scrollArea->backgroundColor = glm::vec4{0.5f, 0.0f, 0.0f, 1.0f};
    scrollArea->setFillBackground(true);

    auto column = scrollArea->appendChild<Column>();
    column->backgroundColor = glm::vec4{0.5f, 0.5f, 0.5f, 1.0f};
    column->setFillBackground(true);
    column->setMargins(kSpacing);
    column->setSpacing(kSpacing);

    constexpr auto kSize = 16;
    for (size_t r = 0; r < kSize; ++r)
    {
        auto row = column->appendChild<Row>();
        row->setFillBackground(false);
        row->setMargins(0.0f);
        row->setSpacing(kSpacing);

        for (size_t c = 0; c < kSize; ++c)
        {
            auto *w = row->appendChild<Rectangle>(50.0f, 50.0f);
            w->backgroundColor =
                glm::vec4{static_cast<float>(r) / (kSize - 1), static_cast<float>(c) / (kSize - 1), 0.0f, 1.0f};
            w->setFillBackground(true);
        }
    }

    m_uiRoot = std::move(outerLayout);
    m_eventManager.setRoot(m_uiRoot.get());
}

void TestWindow::handleWindowSize(const SizeI &size)
{
    m_viewportSize = size;
    m_painter->setViewportSize(size);
}

void TestWindow::handleKey(int key, int scancode, KeyAction action, Modifier mods) {}

void TestWindow::handleMouseButton(MouseButton button, MouseAction action, Modifier mods)
{
    m_eventManager.handleMouseButton(button, action, cursorPos(), mods);
}

void TestWindow::handleMouseMove(const glm::vec2 &pos)
{
    m_eventManager.handleMouseMove(pos);
}

void TestWindow::handleMouseWheel(const glm::vec2 &offset)
{
    m_eventManager.handleMouseWheel(cursorPos(), offset);
}

bool TestWindow::initializeResources()
{
    m_painter = std::make_unique<Painter>();
    return true;
}

void TestWindow::update(Seconds elapsed) {}

void TestWindow::render() const
{
    glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);

    m_painter->begin();
    m_uiRoot->paint(m_painter.get(), glm::vec2{0.0f, 0.0f}, 0);
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
