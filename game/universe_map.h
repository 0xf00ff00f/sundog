#pragma once

#include "universe.h"

#include <cstddef>

#include <glm/glm.hpp>

class Universe;
class Mesh;
class ShaderManager;
class Painter;

class UniverseMap
{
public:
    explicit UniverseMap(const Universe *universe, ShaderManager *shaderManager, Painter *overlayPainter);
    ~UniverseMap();

    void setViewport(int width, int height);
    void render(JulianDate when) const;

private:
    void initializeMeshes();

    const Universe *m_universe{nullptr};
    ShaderManager *m_shaderManager{nullptr};
    Painter *m_overlayPainter;
    int m_viewportWidth{0};
    int m_viewportHeight{0};
    std::unordered_map<const World *, std::unique_ptr<Mesh>> m_orbitMeshes;
    std::unique_ptr<Mesh> m_bodyBillboardMesh;
    glm::mat4 m_projectionMatrix;
};
