#include "system.h"

#include "shader_manager.h"
#include "texture_cache.h"

#include <GLFW/glfw3.h>

#include <print>

System *System::s_instance = nullptr;

System::System()
{
    s_instance = this;

    if (!glfwInit())
    {
        std::println(stderr, "Failed to initialized GLFW");
        std::abort();
    }
}

System::~System()
{
    m_textureCache.reset();
    m_shaderManager.reset();

    glfwTerminate();
}

bool System::initializeResources()
{
    m_shaderManager = std::make_unique<ShaderManager>();
    if (!m_shaderManager->initialize())
        return false;

    m_textureCache = std::make_unique<TextureCache>();

    return true;
}
