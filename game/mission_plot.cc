#include "mission_plot.h"

#include <mdspan>

Image32 createMissionPlot(const MissionTable &table)
{
    const auto width = table.departures.size();
    const auto height = table.arrivals.size();
    Image32 image(width, height);

    double minDeltaV = std::numeric_limits<double>::max();
    double maxDeltaV = std::numeric_limits<double>::lowest();
    for (const auto &orbit : table.transferOrbits)
    {
        if (orbit)
        {
            const auto deltaV = orbit->deltaVDeparture + orbit->deltaVArrival;
            minDeltaV = std::min(minDeltaV, deltaV);
            maxDeltaV = std::max(maxDeltaV, deltaV);
        }
    }

    auto gradientColor = [minDeltaV, maxDeltaV](double deltaV) -> glm::vec3 {
        constexpr auto gradient = std::array{glm::vec3{0, 0, 1}, glm::vec3{0, 1, 1}, glm::vec3{0, 1, 0},
                                             glm::vec3{1, 1, 0}, glm::vec3{1, 0, 0}};
        assert(minDeltaV < maxDeltaV);
        assert(deltaV >= minDeltaV && deltaV <= maxDeltaV);
        if (deltaV == maxDeltaV)
            return gradient.back();
        const auto t = ((deltaV - minDeltaV) / (maxDeltaV - minDeltaV)) * (gradient.size() - 1);
        const auto index = static_cast<std::size_t>(t);
        assert(index < gradient.size() - 1);
        return glm::mix(gradient[index], gradient[index + 1], t - std::floor(t));
    };

    assert(table.transferOrbits.size() == width * height);
    auto orbits = std::mdspan(table.transferOrbits.data(), height, width);
    auto pixels = std::mdspan(reinterpret_cast<glm::u8vec4 *>(image.pixels().data()), height, width);
    for (std::size_t y = 0; y < orbits.extent(0); ++y)
    {
        for (std::size_t x = 0; x < orbits.extent(1); ++x)
        {
            const auto color = [&] {
                if (const auto orbit = orbits[y, x])
                    return gradientColor(orbit->deltaVDeparture + orbit->deltaVArrival);
                else
                    return glm::vec3{1.0f};
            }();
            pixels[y, x] = glm::u8vec4{color * 255.f, 255};
        }
    }

    return image;
}
