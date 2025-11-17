#include <shader_manager.h>
#include <window_base.h>
#include <gui.h>
#include <painter.h>

#include <GLFW/glfw3.h>

#include <glm/gtx/string_cast.hpp>

#include <print>

using namespace ui;

class Button : public Gizmo
{
public:
    explicit Button(std::string_view name, Gizmo *parent = nullptr);

    bool handleMousePress(const glm::vec2 &position) override;
    void handleMouseRelease(const glm::vec2 &position) override;
    void handleMouseMove(const glm::vec2 &position) override;
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
    hoverable = true;
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
    void handleMouseMove(const glm::vec2 &position) override;

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
    auto row = std::make_unique<Row>();
    row->backgroundColor = glm::vec4{0.0f, 1.0f, 0.0f, 1.0f};
    row->setMargins(Margins{10.0f, 10.0f, 10.0f, 10.0f});
    row->setSpacing(10.0f);

    row->appendChild<Button>("Button 1");
    row->appendChild<Button>("Button 2");
    row->appendChild<Button>("Button 3");

    m_uiRoot = std::move(row);

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
