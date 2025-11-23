#include <base/shader_manager.h>
#include <base/window_base.h>
#include <base/gui.h>
#include <base/painter.h>

#include <GLFW/glfw3.h>

#include <glm/gtx/string_cast.hpp>

#include <print>

using namespace ui;

class Button : public Gizmo
{
public:
    explicit Button(std::string_view name, Gizmo *parent = nullptr);

    bool handleMousePress(const glm::vec2 &pos) override;
    void handleMouseRelease(const glm::vec2 &pos) override;
    void handleMouseMove(const glm::vec2 &pos) override;
    void handleMouseEnter() override;
    void handleMouseLeave() override;

private:
    std::string m_name;
};

Button::Button(std::string_view name, Gizmo *parent)
    : Gizmo(parent)
    , m_name(name)
{
    backgroundColor = glm::vec4{1.0, 0.0, 0.0, 1.0};
    setHoverable(true);
    setSize(SizeF{120.0, 80.0});
}

bool Button::handleMousePress(const glm::vec2 &pos)
{
    assert(pos.x >= 0.0f && pos.x < m_size.width());
    assert(pos.y >= 0.0f && pos.y < m_size.height());
    std::println("**** {}: mousePress: pos={}", m_name, glm::to_string(pos));
    return true;
}

void Button::handleMouseRelease(const glm::vec2 &pos)
{
    std::println("**** {}: mouseRelease: pos={}", m_name, glm::to_string(pos));
}

void Button::handleMouseMove(const glm::vec2 &pos)
{
    std::println("**** {}: mouseMove: pos={}", m_name, glm::to_string(pos));
}

void Button::handleMouseEnter()
{
    backgroundColor = glm::vec4{1.0, 0.5, 0.5, 1.0};
}

void Button::handleMouseLeave()
{
    backgroundColor = glm::vec4{1.0, 0.0, 0.0, 1.0};
}

class TestWindow : public WindowBase
{
public:
    TestWindow();

    void handleWindowSize(const SizeI &size) override;
    void handleKey(int key, int scancode, KeyAction action, Modifier mods) override;
    void handleMouseButton(MouseButton button, MouseAction action, Modifier mods) override;
    void handleMouseMove(const glm::vec2 &pos) override;

private:
    bool initializeResources() override;
    void update(Seconds elapsed) override;
    void render() const override;

    SizeI m_viewportSize;
    std::unique_ptr<ShaderManager> m_shaderManager;
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

    auto scrollArea = outerLayout->appendChild<ScrollArea>(400.0f, 400.0f);
    scrollArea->backgroundColor = glm::vec4{0.5f, 0.0f, 0.0f, 1.0f};

    auto column = scrollArea->appendChild<Column>();
    column->backgroundColor = glm::vec4{0.5f, 0.5f, 0.5f, 1.0f};
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
