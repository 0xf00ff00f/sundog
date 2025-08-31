#include "file.h"

#include <cstdio>

std::vector<std::byte> readFile(const std::filesystem::path &path)
{
    auto stream = std::unique_ptr<FILE, decltype(&fclose)>(fopen(path.string().c_str(), "rb"), &fclose);
    if (!stream)
        return {};
    fseek(stream.get(), 0l, SEEK_END);
    const auto size = ftell(stream.get());
    fseek(stream.get(), 0l, SEEK_SET);
    std::vector<std::byte> buffer(size);
    if (fread(buffer.data(), 1, size, stream.get()) != size)
        return {};
    return buffer;
}
