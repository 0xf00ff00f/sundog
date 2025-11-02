#pragma once

#include "rect.h"
#include "universe.h"
#include "window_base.h"
#include "camera_controller.h"

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

    void setViewportSize(const SizeI &size);
    void render(JulianDate when) const;

    void handleMouseButton(MouseButton button, MouseAction action, const glm::vec2 &position, Modifier mods);
    void handleMouseMove(const glm::vec2 &position);

    void update(Seconds elapsed);

private:
    void initializeMeshes();

    const Universe *m_universe{nullptr};
    ShaderManager *m_shaderManager{nullptr};
    Painter *m_overlayPainter;
    SizeI m_viewportSize;
    std::unordered_map<const World *, std::unique_ptr<Mesh>> m_orbitMeshes;
    std::unique_ptr<Mesh> m_bodyBillboardMesh;
    glm::mat4 m_projectionMatrix;
    CameraController m_cameraController;
};
