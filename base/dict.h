#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

struct TransparentStringHash
{
    using is_transparent = void;
    size_t operator()(const char *s) const { return std::hash<std::string_view>{}(s); }
    size_t operator()(std::string_view s) const { return std::hash<std::string_view>{}(s); }
    size_t operator()(const std::string &s) const { return std::hash<std::string_view>{}(s); }
};

template<typename T>
using Dict = std::unordered_map<std::string, T, TransparentStringHash, std::equal_to<>>;
