#pragma once

#include <glm/glm.hpp>

#include <tuple>

enum class OrbitType
{
    Prograde,
    Retrograde
};

std::optional<std::tuple<glm::dvec3, glm::dvec3>> lambert_battin(double mu, const glm::dvec3 &r1, const glm::dvec3 &r2,
                                                                 double dt, OrbitType ot = OrbitType::Prograde);
