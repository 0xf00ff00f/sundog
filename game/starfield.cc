#include "starfield.h"

#include <base/file.h>

#include <base/asset_path.h>

#include <glm/glm.hpp>

#include <nlohmann/json.hpp>

Star::SpectralClass toSpectralClass(std::string_view spectralClass)
{
    if (!spectralClass.empty())
    {
        switch (spectralClass[0])
        {
        case 'O':
            return Star::SpectralClass::O;
        case 'B':
            return Star::SpectralClass::B;
        case 'A':
            return Star::SpectralClass::A;
        case 'F':
            return Star::SpectralClass::F;
        case 'G':
            return Star::SpectralClass::G;
        case 'K':
            return Star::SpectralClass::K;
        case 'M':
            return Star::SpectralClass::M;
        }
    }
    return Star::SpectralClass::A;
}

void from_json(const nlohmann::json &json, Star &star)
{
    star.bayerName = json.at("bayer_name").get<std::string>();
    star.properName = json.at("proper_name").get<std::string>();
    star.rightAscension = glm::radians(json.at("right_ascension").get<float>());
    star.declination = glm::radians(json.at("declination").get<float>());
    star.spectralClass = toSpectralClass(json.at("spectral_type").get<std::string>());
    star.apparentMagnitude = json.at("apparent_magnitude").get<float>();
}

bool Starfield::load(const std::string &path)
{
    const auto jsonData = readFile(path);
    if (jsonData.empty())
        return false;
    const nlohmann::json json = nlohmann::json::parse(jsonData);
    stars = json.get<std::vector<Star>>();
    return true;
}
