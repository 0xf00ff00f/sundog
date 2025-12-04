#include "asset_path.h"

#include <filesystem>

namespace
{

std::filesystem::path assetsDir()
{
    static const auto path = std::filesystem::path{ASSETSDIR};
    return path;
}

} // namespace

std::string shaderFilePath(std::string_view name)
{
    return (assetsDir() / "shaders" / name).string();
}

std::string fontFilePath(std::string_view name)
{
    return (assetsDir() / "fonts" / name).string();
}

std::string dataFilePath(std::string_view name)
{
    return (assetsDir() / "data" / name).string();
}

std::string imageFilePath(std::string_view name)
{
    return (assetsDir() / "images" / name).string();
}
