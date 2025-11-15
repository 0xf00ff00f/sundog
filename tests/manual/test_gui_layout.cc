#include <shader_manager.h>
#include <window_base.h>
#include <gui.h>
#include <painter.h>

#include <GLFW/glfw3.h>

#include <print>

class TestWindow : public WindowBase
{
public:
    TestWindow();

    void handleWindowSize(const SizeI &size) override;
    void handleKey(int key, int scancode, KeyAction action, Modifier mods) override;
    void handleMouseButton(MouseButton button, MouseAction action, Modifier mods) override;
    void handleMouseMove(const glm::vec2 &position) override;

private:
    bool initializeResources() override;
    void update(Seconds elapsed) override;
    void render() const override;

    SizeI m_viewportSize;
    std::unique_ptr<ShaderManager> m_shaderManager;
    std::unique_ptr<Painter> m_painter;
    std::unique_ptr<Gizmo> m_uiRoot;
};

TestWindow::TestWindow()
{
    constexpr auto kRed = glm::vec4{1.0, 0.0, 0.0, 1.0};
    constexpr auto kGreen = glm::vec4{0.0, 1.0, 0.0, 1.0};
    constexpr auto kBlue = glm::vec4{0.0, 0.0, 1.0, 1.0};

    auto row = std::make_unique<Row>();
    row->backgroundColor = kRed;

    auto *r1 = row->appendChild<Rectangle>(60.0, 100.0);
    r1->backgroundColor = kGreen;

    auto *r2 = row->appendChild<Rectangle>(100.0, 60.0);
    r2->backgroundColor = kBlue;

    auto *r3 = row->appendChild<Rectangle>(100.0, 60.0);
    r3->backgroundColor = kGreen;

    auto *col = row->appendChild<Column>();
    col->backgroundColor = kBlue;

    auto *r4 = col->appendChild<Rectangle>(100.0, 20.0);
    r4->backgroundColor = kRed;

    auto *r5 = col->appendChild<Rectangle>(100.0, 20.0);
    r5->backgroundColor = kRed;

    m_uiRoot = std::move(row);
}

void TestWindow::handleWindowSize(const SizeI &size)
{
    m_viewportSize = size;
    m_painter->setViewportSize(size);
}

void TestWindow::handleKey(int key, int scancode, KeyAction action, Modifier mods) {}

void TestWindow::handleMouseButton(MouseButton button, MouseAction action, Modifier mods) {}

void TestWindow::handleMouseMove(const glm::vec2 &position) {}

bool TestWindow::initializeResources()
{
    m_shaderManager = std::make_unique<ShaderManager>();
    if (!m_shaderManager->initialize())
        return false;

    m_painter = std::make_unique<Painter>(m_shaderManager.get());

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
    if (!glfwInit())
    {
        std::println(stderr, "Failed to initialized GLFW");
        return 1;
    }

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
