#include "shader_manager.h"

#include "asset_path.h"
#include "file.h"

namespace
{
std::optional<gl::ShaderProgram> initializeShaderProgram(std::string_view vertexShader, std::string_view fragmentShader)
{
    gl::ShaderProgram program;
    if (!program.attachShader(gl::ShaderProgram::ShaderType::FragmentShader, fragmentShader))
        return {};
    if (!program.attachShader(gl::ShaderProgram::ShaderType::VertexShader, vertexShader))
        return {};
    if (!program.link())
        return {};
    return program;
}
} // namespace

ShaderManager::ShaderManager() = default;

ShaderManager::~ShaderManager() = default;

bool ShaderManager::initialize()
{
    struct ShaderInfo
    {
        std::string vsPath;
        std::string fsPath;
    };
    static const std::array<ShaderInfo, static_cast<size_t>(Shader::Count)> shaders = {
        {{"wireframe.vert", "wireframe.frag"},
         {"billboard.vert", "billboard.frag"},
         {"flat.vert", "flat.frag"},
         {"text.vert", "text.frag"},
         {"orbit.vert", "orbit.frag"},
         {"partial_orbit.vert", "partial_orbit.frag"}}};
    for (size_t index = 0; const auto &[vsPath, fsPath] : shaders)
    {
        const auto vertexShader = readFile(shaderFilePath(vsPath));
        if (vertexShader.empty())
            return false;
        const auto fragmentShader = readFile(shaderFilePath(fsPath));
        if (fragmentShader.empty())
            return false;

        const auto asStringView = [](std::span<const std::byte> bytes) {
            return std::string_view{reinterpret_cast<const char *>(bytes.data()), bytes.size()};
        };
        auto program =
            initializeShaderProgram(asStringView(std::span{vertexShader}), asStringView(std::span{fragmentShader}));
        if (!program)
            return false;

        auto &cachedProgram = m_programs[index];
        cachedProgram.program = std::move(*program);
        cachedProgram.uniformLocations.fill(-1);

        ++index;
    }

    return true;
}

void ShaderManager::setCurrent(Shader shader)
{
    auto *cachedShader = &m_programs[static_cast<size_t>(shader)];
    if (cachedShader == m_currentShader)
        return;
    m_currentShader = cachedShader;
    cachedShader->program.use();
}

int ShaderManager::uniformLocation(Uniform uniform)
{
    if (!m_currentShader)
        return -1;
    auto &locations = m_currentShader->uniformLocations;
    const auto index = static_cast<size_t>(uniform);
    if (locations[index] == -1)
    {
        static const std::array<std::string, static_cast<size_t>(Uniform::Count)> uniforms = {
            "projectionMatrix", "viewMatrix", "modelMatrix",  "mvp",      "color",       "semiMajorAxis",
            "eccentricity",     "startAngle", "currentAngle", "endAngle", "aspectRatio", "thickness"};
        locations[index] = m_currentShader->program.uniformLocation(uniforms[index]);
    }
    return locations[index];
}
