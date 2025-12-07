#include "camera_controller.h"

#include <glm/gtx/string_cast.hpp>

#include <print>

namespace
{
constexpr auto kTargetAnimationMinDistance = 0.001f;
}

VelocitySampler::VelocitySampler() = default;

void VelocitySampler::reset(const glm::vec2 &position)
{
    m_lastPosition = position;
    m_sampleCount = 0;
}

void VelocitySampler::addSample(const glm::vec2 &position, Seconds elapsed)
{
    const auto velocity = (position - m_lastPosition) / static_cast<float>(elapsed.count());
    m_buffer[m_sampleCount % m_buffer.size()] = velocity;
    ++m_sampleCount;
    m_lastPosition = position;
}

glm::vec2 VelocitySampler::velocity() const
{
    const auto samples = std::min(m_sampleCount, m_buffer.size());
    auto velocity = glm::vec2{0.0f};
    std::size_t index = (m_sampleCount + m_buffer.size() - 1) % m_buffer.size();
    for (std::size_t i = 0; i < samples; ++i)
    {
        velocity += m_buffer[index];
        index = (index + m_buffer.size() - 1) % m_buffer.size();
    }
    velocity *= 1.0f / samples;
    return velocity;
}

CameraController::CameraController()
{
    updateViewMatrix();
}

void CameraController::setViewportSize(const SizeI &size)
{
    m_viewportSize = size;
}

void CameraController::handleMouseButton(MouseButton button, MouseAction action, const glm::vec2 &pos, Modifier mods)
{
    if (button == MouseButton::Left)
    {
        switch (action)
        {
        case MouseAction::Press: {
            m_dragging = true;
            m_lastPosition = normalizedViewportPos(pos);
            m_mouseVelocitySampler.reset(m_lastPosition);
            break;
        }
        case MouseAction::Release: {
            m_dragging = false;
            m_rotationVelocity = m_mouseVelocitySampler.velocity();
            break;
        }
        }
    }
}

void CameraController::handleMouseMove(const glm::vec2 &viewportPos)
{
    if (m_dragging)
    {
        const auto pos = normalizedViewportPos(viewportPos);
        const auto offset = pos - m_lastPosition;
        rotate(offset);
        m_lastPosition = pos;
    }
}

void CameraController::updateViewMatrix()
{
    m_viewMatrix = glm::lookAt(m_cameraEye, m_cameraCenter, m_upDir);
}

glm::vec2 CameraController::normalizedViewportPos(const glm::vec2 &pos) const
{
    return pos / glm::vec2{m_viewportSize.width(), m_viewportSize.height()};
}

void CameraController::update(Seconds seconds)
{
    if (m_dragging)
    {
        m_mouseVelocitySampler.addSample(m_lastPosition, seconds);
    }
    else
    {
        if (glm::length(m_rotationVelocity) > 0.001f)
        {
            rotate(m_rotationVelocity * static_cast<float>(seconds.count()));
            m_rotationVelocity *= std::expf(-5.0f * static_cast<float>(seconds.count()));
        }
    }

    if (m_targetCameraCenter.has_value())
    {
        const auto alpha = std::expf(-100.f * static_cast<float>(seconds.count()));
        const auto updatedCameraCenter = alpha * m_targetCameraCenter.value() + (1.0f - alpha) * m_cameraCenter;
        if (glm::distance(updatedCameraCenter, m_targetCameraCenter.value()) < kTargetAnimationMinDistance)
        {
            m_cameraCenter = m_targetCameraCenter.value();
            m_targetCameraCenter.reset();
        }
        moveCameraCenter(updatedCameraCenter);
    }
}

void CameraController::rotate(const glm::vec2 &dir)
{
    const auto angle = -dir * m_angularSpeed;

    constexpr auto kMinPitch = 0.1f;
    constexpr auto kMaxPitch = 0.5f * glm::pi<float>();

    auto eyeDir = m_cameraEye - m_cameraCenter;
    const auto cameraDistance = glm::length(eyeDir);
    eyeDir = glm::normalize(eyeDir);

    // rotate pitch, limit it to 0-90 degrees
    const auto cameraRight = glm::normalize(glm::cross(eyeDir, m_upDir));
    const auto projDir = glm::normalize(glm::cross(m_upDir, cameraRight));
    const auto pitch = std::acos(glm::clamp(glm::dot(eyeDir, m_upDir), -1.0f, 1.0f));
    const auto rotatedPitch = glm::clamp(pitch + angle.y, kMinPitch, kMaxPitch);
    auto rotatedEyeDir = projDir * glm::sin(rotatedPitch) + m_upDir * std::cos(rotatedPitch);

    // rotate around up vector
    const auto r = glm::rotate(glm::mat4{1.0}, angle.x, m_upDir);
    rotatedEyeDir = glm::vec3(r * glm::vec4(rotatedEyeDir, 0.0));

    m_cameraEye = m_cameraCenter + cameraDistance * glm::normalize(rotatedEyeDir);

    updateViewMatrix();
}

void CameraController::setCameraCenter(const glm::vec3 &cameraCenter)
{
    m_cameraCenter = cameraCenter;
    updateViewMatrix();
}

void CameraController::moveCameraCenter(const glm::vec3 &cameraCenter, bool animate)
{
    if (animate && glm::distance(cameraCenter, m_cameraCenter) >= kTargetAnimationMinDistance)
    {
        m_targetCameraCenter = cameraCenter;
    }
    else
    {
        moveCameraCenter(cameraCenter);
    }
}

void CameraController::moveCameraCenter(const glm::vec3 &cameraCenter)
{
    const auto eyeToCamera = m_cameraEye - m_cameraCenter;
    m_cameraCenter = cameraCenter;
    m_cameraEye = m_cameraCenter + eyeToCamera;
    updateViewMatrix();
}

void CameraController::setUpDir(const glm::vec3 &upDir)
{
    m_upDir = upDir;
    updateViewMatrix();
}
