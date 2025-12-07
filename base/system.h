#pragma once

#include <memory>

class ShaderManager;
class TextureCache;

class System
{
public:
    System();
    ~System();

    ShaderManager *shaderManager() { return m_shaderManager.get(); }
    TextureCache *textureCache() { return m_textureCache.get(); }

    static System *instance() { return s_instance; }

    // call this after we have an OpenGL context
    bool initializeResources();

private:
    static System *s_instance;
    std::unique_ptr<ShaderManager> m_shaderManager;
    std::unique_ptr<TextureCache> m_textureCache;
};
