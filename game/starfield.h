#pragma once

#include <string>
#include <vector>

struct Star
{
    enum class SpectralClass
    {
        O,
        B,
        A,
        F,
        G,
        K,
        M
    };
    std::string bayerName;
    std::string properName;
    float rightAscension;
    float declination;
    SpectralClass spectralClass;
    float apparentMagnitude;
};

struct Starfield
{
    std::vector<Star> stars;

    bool load(const std::string &path);
};
