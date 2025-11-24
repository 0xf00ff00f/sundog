#pragma once

#include "universe.h"
#include "camera_controller.h"

#include <base/rect.h>
#include <base/window_base.h>

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

    void handleMouseButton(MouseButton button, MouseAction action, const glm::vec2 &pos, Modifier mods);
    void handleMouseMove(const glm::vec2 &pos);

    void update(Seconds elapsed);

private:
    void initializeMeshes();

    const Universe *m_universe{nullptr};
    ShaderManager *m_shaderManager{nullptr};
    Painter *m_overlayPainter;
    SizeI m_viewportSize;
    std::unique_ptr<Mesh> m_bodyBillboardMesh;
    std::unique_ptr<Mesh> m_orbitMesh;
    glm::mat4 m_projectionMatrix;
    CameraController m_cameraController;
};
