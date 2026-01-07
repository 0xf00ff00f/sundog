#pragma once

#include "glhelpers.h"

#include <array>

class ShaderManager
{
public:
    enum class Shader
    {
        Wireframe,
        Billboard,
        Flat,
        Text,
        Orbit,
        PartialOrbit,
        Planet,
        Starfield,
        Count
    };

    enum class Uniform
    {
        ProjectionMatrix,
        ViewMatrix,
        ModelMatrix,
        ModelViewMatrix,
        ModelViewNormalMatrix,
        ModelViewProjectionMatrix,
        Color,
        SemiMajorAxis,
        Eccentricity,
        StartAngle,
        CurrentAngle,
        EndAngle,
        VertexCount,
        AspectRatio,
        Thickness,
        LightPosition,
        LightIntensity,
        Ambient,
        Specular,
        Shininess,
        Count
    };

    ShaderManager();
    ~ShaderManager();

    ShaderManager(ShaderManager &&) = delete;
    ShaderManager &operator=(ShaderManager &&) = delete;

    ShaderManager(const ShaderManager &) = delete;
    ShaderManager &operator=(const ShaderManager &) = delete;

    bool initialize();
    void setCurrent(Shader shader);
    int uniformLocation(Uniform uniform);

    template<typename T>
    void setUniform(Uniform uniform, T &&value)
    {
        if (!m_currentShader)
            return;
        const auto location = uniformLocation(uniform);
        if (location == -1)
            return;
        m_currentShader->program.setUniform(location, std::forward<T>(value));
    }

private:
    struct CachedShader
    {
        gl::ShaderProgram program;
        std::array<int, static_cast<size_t>(Uniform::Count)> uniformLocations;
    };
    CachedShader *m_currentShader = nullptr;
    std::array<CachedShader, static_cast<size_t>(Shader::Count)> m_programs;
};
