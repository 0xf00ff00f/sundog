#include "camera_controller.h"

#include <glm/gtx/string_cast.hpp>

#include <print>

CameraController::CameraController()
{
    updateViewMatrix();
}

void CameraController::setViewportSize(const SizeI &size)
{
    m_viewportSize = size;
}

void CameraController::handleMouseButton(MouseButton button, MouseAction action, const glm::vec2 &position,
                                         Modifier mods)
{
    if (button == MouseButton::Left)
    {
        switch (action)
        {
        case MouseAction::Press: {
            m_dragging = true;
            m_lastPosition = normalizedViewportPos(position);
            break;
        }
        case MouseAction::Release: {
            m_dragging = false;
            break;
        }
        }
    }
}

void CameraController::handleMouseMove(const glm::vec2 &viewportPos)
{
    if (m_dragging)
    {
        constexpr auto kMinPitch = 0.1f;
        constexpr auto kMaxPitch = 0.5f * glm::pi<float>();

        const auto pos = normalizedViewportPos(viewportPos);
        const auto offset = (pos - m_lastPosition) * m_rotationSpeed;

        auto eyeDir = m_cameraEye - m_cameraCenter;
        const auto cameraDistance = glm::length(eyeDir);
        eyeDir = glm::normalize(eyeDir);

        // rotate pitch, limit it to 0-90 degrees
        const auto cameraRight = glm::normalize(glm::cross(eyeDir, m_upDir));
        const auto projDir = glm::normalize(glm::cross(m_upDir, cameraRight));
        const auto pitch = std::acos(glm::clamp(glm::dot(eyeDir, m_upDir), -1.0f, 1.0f));
        const auto rotatedPitch = glm::clamp(pitch - offset.y, kMinPitch, kMaxPitch);
        auto rotatedEyeDir = projDir * glm::sin(rotatedPitch) + m_upDir * std::cos(rotatedPitch);

        // rotate around up vector
        const auto r = glm::rotate(glm::mat4{1.0}, -offset.x, m_upDir);
        rotatedEyeDir = glm::vec3(r * glm::vec4(rotatedEyeDir, 0.0));

        m_cameraEye = m_cameraCenter + cameraDistance * glm::normalize(rotatedEyeDir);

        updateViewMatrix();
        m_lastPosition = pos;
    }
}

void CameraController::updateViewMatrix()
{
    m_viewMatrix = glm::lookAt(m_cameraEye, m_cameraCenter, m_upDir);
}

glm::vec2 CameraController::normalizedViewportPos(const glm::vec2 &position) const
{
    return position / glm::vec2{m_viewportSize.width(), m_viewportSize.height()};
}
