#pragma once

#include "window_base.h"

class CameraController
{
public:
    CameraController();

    void setViewportSize(const SizeI &size);

    void handleMouseButton(MouseButton button, MouseAction action, const glm::vec2 &position, Modifier mods);
    void handleMouseMove(const glm::vec2 &position);

    glm::mat4 viewMatrix() const { return m_viewMatrix; }

private:
    glm::vec2 normalizedViewportPos(const glm::vec2 &viewportPos) const;
    void updateViewMatrix();

    glm::mat4 m_viewMatrix = glm::mat4{1.0f};
    glm::vec3 m_cameraCenter = glm::vec3{0.0f};
    glm::vec3 m_cameraEye = glm::vec3{0.0f, -4.0f, 4.0f};
    glm::vec3 m_upDir = glm::vec3{0.0f, 0.0f, 1.0f};
    glm::vec2 m_rotationSpeed = glm::vec2{5.0f};
    bool m_dragging{false};
    glm::vec2 m_lastPosition;
    SizeI m_viewportSize;
};
