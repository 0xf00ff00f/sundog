#pragma once

#include <glm/glm.hpp>

#include <tuple>

enum class OrbitType
{
    Prograde,
    Retrograde
};

struct TransferVelocities
{
    glm::dvec3 initialVelocity;
    glm::dvec3 finalVelocity;
};
std::optional<TransferVelocities> lambert_battin(double mu, const glm::dvec3 &r1, const glm::dvec3 &r2, double dt,
                                                 OrbitType ot = OrbitType::Prograde);
