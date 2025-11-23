#include <base/shader_manager.h>
#include <base/window_base.h>
#include <base/gui.h>
#include <base/painter.h>

#include <GLFW/glfw3.h>

#include <print>

using namespace ui;

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
    MultiLineText *m_text{nullptr};
    double m_time{0.0};
};

TestWindow::TestWindow()
{
    constexpr auto kRed = glm::vec4{0.8, 0.6, 0.6, 1.0};
    constexpr auto kGreen = glm::vec4{0.6, 0.8, 0.6, 1.0};
    constexpr auto kBlue = glm::vec4{0.6, 0.6, 0.8, 1.0};
    constexpr auto kWhite = glm::vec4{1.0};

    auto row = std::make_unique<Row>();
    row->backgroundColor = kWhite;
    row->setFillBackground(true);
    row->setMargins(8.0f);

    auto *r1 = row->appendChild<Rectangle>(60.0, 300.0);
    r1->setAlign(Align::Top);
    r1->backgroundColor = kGreen;
    r1->setFillBackground(true);

    auto *r2 = row->appendChild<Rectangle>(60.0, 300.0);
    r2->setAlign(Align::VerticalCenter);
    r2->backgroundColor = kBlue;
    r2->setFillBackground(true);

    auto *r3 = row->appendChild<Rectangle>(60.0, 300.0);
    r3->setAlign(Align::Bottom);
    r3->backgroundColor = kGreen;
    r3->setFillBackground(true);

    auto *col = row->appendChild<Column>();
    col->backgroundColor = kBlue;
    col->setFillBackground(true);
    col->setMargins(8.0f);
    col->setMinimumWidth(180.0);

    auto *r4 = col->appendChild<Rectangle>(100.0, 60.0);
    r4->setAlign(Align::Left);
    r4->backgroundColor = kRed;
    r4->setFillBackground(true);

    auto *r5 = col->appendChild<Rectangle>(100.0, 60.0);
    r5->setAlign(Align::HorizontalCenter);
    r5->backgroundColor = kRed;
    r5->setFillBackground(true);

    auto *r6 = col->appendChild<Rectangle>(100.0, 60.0);
    r6->setAlign(Align::Right);
    r6->backgroundColor = kRed;
    r6->setFillBackground(true);

    const Font font{"DejaVuSans.ttf", 16.0f, 0};

    auto *frame = col->appendChild<Row>();
    frame->backgroundColor = glm::vec4{1.0f};
    frame->setFillBackground(true);
    frame->setMargins(8.0);

    m_text = frame->appendChild<MultiLineText>();
    m_text->setFont(font);
    m_text->setText(
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris dui eros, scelerisque et consectetur et, "
        "molestie id mauris. Mauris quis ante quis tellus mollis volutpat sed nec metus. Duis quis dolor velit. Mauris "
        "sem quam, gravida eu orci et, fermentum vulputate nulla. Praesent interdum semper lacinia. Pellentesque "
        "habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Aenean et placerat ante. "
        "Pellentesque lacinia sem nec cursus sodales. Aenean volutpat urna eleifend libero vehicula viverra. Mauris "
        "vehicula mi a hendrerit sagittis. In nec ante quis enim cursus semper.");
    m_text->setLineWidth(120.0f);
    m_text->color = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f};

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

void TestWindow::update(Seconds elapsed)
{
    assert(m_text);
    m_time += elapsed.count();
    m_text->setLineWidth(300.0f + 120.0f * std::sin(m_time));
}

void TestWindow::render() const
{
    glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
