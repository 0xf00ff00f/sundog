#include "style_settings.h"

constexpr auto kNormalFontFamily{"DejaVuSans.ttf"};
constexpr auto kBoldFontFamily{"DejaVuSans-Bold.ttf"};

StyleSettings g_styleSettings = [] {
    StyleSettings settings{.smallFont = Font{kNormalFontFamily, 16.0f, 0},
                           .normalFont = Font{kNormalFontFamily, 20.0f, 0},
                           .titleFont = Font{kBoldFontFamily, 30.0f, 0},
                           .baseColor = glm::vec4{1.0f, 0.75f, 0.0f, 1.0f},
                           .accentColor = glm::vec4{1.0f}};
    return settings;
}();
