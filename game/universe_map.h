#pragma once

#include "universe.h"
#include "camera_controller.h"

#include <base/rect.h>
#include <base/window_base.h>

#include <cstddef>

#include <glm/glm.hpp>

class Universe;
class Mesh;
class Painter;

class UniverseMap
{
public:
    explicit UniverseMap(const Universe *universe, Painter *overlayPainter);
    ~UniverseMap();

    void setViewportSize(const SizeI &size);
    void render() const;

    void handleMouseButton(MouseButton button, MouseAction action, const glm::vec2 &pos, Modifier mods);
    void handleMouseMove(const glm::vec2 &pos);
    void handleMouseWheel(const glm::vec2 &mousePos, const glm::vec2 &wheelOffset);

    void update(Seconds elapsed);

private:
    void initializeMeshes();
    const World *pickWorld(const glm::vec2 &viewportPos);

    const Universe *m_universe{nullptr};
    Painter *m_overlayPainter;
    SizeI m_viewportSize;
    std::unique_ptr<Mesh> m_circleBillboardMesh;
    std::unique_ptr<Mesh> m_sphereMesh;
    std::unique_ptr<Mesh> m_orbitMesh;
    glm::mat4 m_projectionMatrix;
    CameraController m_cameraController;
    const World *m_cameraTarget{nullptr};
};
