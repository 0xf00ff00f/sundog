#pragma once

#include "window_base.h"

struct VelocitySampler
{
    VelocitySampler();

    void reset(const glm::vec2 &position);
    void addSample(const glm::vec2 &position, Seconds elapsed);

    glm::vec2 velocity() const;

    glm::vec2 m_lastPosition;
    Seconds m_lastSampleTime;
    std::size_t m_sampleCount = 0;
    std::array<glm::vec2, 80> m_buffer;
};

class CameraController
{
public:
    CameraController();

    void setViewportSize(const SizeI &size);

    void handleMouseButton(MouseButton button, MouseAction action, const glm::vec2 &position, Modifier mods);
    void handleMouseMove(const glm::vec2 &position);

    void update(Seconds seconds);

    glm::mat4 viewMatrix() const { return m_viewMatrix; }

private:
    glm::vec2 normalizedViewportPos(const glm::vec2 &viewportPos) const;
    void updateViewMatrix();
    void rotate(const glm::vec2 &dir);

    glm::mat4 m_viewMatrix = glm::mat4{1.0f};
    glm::vec3 m_cameraCenter = glm::vec3{0.0f};
    glm::vec3 m_cameraEye = glm::vec3{0.0f, -4.0f, 4.0f};
    glm::vec3 m_upDir = glm::vec3{0.0f, 0.0f, 1.0f};
    glm::vec2 m_angularSpeed = glm::vec2{5.0f};
    bool m_dragging{false};
    glm::vec2 m_lastPosition;
    VelocitySampler m_mouseVelocitySampler;
    glm::vec2 m_rotationVelocity = glm::vec2{0.0f};
    SizeI m_viewportSize;
};
