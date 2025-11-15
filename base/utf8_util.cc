#include "utf8_util.h"

#include <codecvt>
#include <locale>

// TODO: codecvt_utf8 and wstring_convert are deprecated since C++17, find a portable alternative

std::u32string decodeUtf8(std::string_view utf8)
{
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
    return conv.from_bytes(utf8.data());
}
