#pragma once

#include <base/font.h>

#include <glm/glm.hpp>

struct StyleSettings
{
    Font smallFont;
    Font normalFont;
    Font titleFont;
    glm::vec4 baseColor;
    glm::vec4 accentColor;
};

extern StyleSettings g_styleSettings;
