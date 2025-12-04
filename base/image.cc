#include "image.h"

#include "file.h"
#include "asset_path.h"
#include "dict.h"

#include <stb_image.h>

#include <cassert>
#include <print>
#include <unordered_map>
#include <algorithm>

Image32 loadImage(const std::string &path, bool flip)
{
    if (flip)
        stbi_set_flip_vertically_on_load(1);

    const auto buffer = readFile(path);
    if (buffer.empty())
        return {};

    int width, height, channels;
    auto *data = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(buffer.data()),
                                       static_cast<int>(buffer.size()), &width, &height, &channels, 4);
    if (!data)
        return {};

    Image32 image(width, height);
    assert(image.pixels().size() == width * height);
    std::memcpy(image.pixels().data(), data, width * height * sizeof(uint32_t));
    stbi_image_free(data);

    return image;
}

const Image<uint32_t> *findOrCreateImage(std::string_view name)
{
    static Dict<std::unique_ptr<Image32>> cache;
    auto it = cache.find(name);
    if (it == cache.end())
    {
        auto image = loadImage(imageFilePath(name));
        it = cache.insert(it, {std::string{name}, std::make_unique<Image32>(std::move(image))});
    }
    return it->second.get();
}
