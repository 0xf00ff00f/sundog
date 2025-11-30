#include "util.h"

#include <locale>
#include <format>

namespace
{

std::locale thousandsSepLocale()
{
    struct ThousandsSepPunct : std::numpunct<char>
    {
        char do_thousands_sep() const override { return ','; }
        std::string do_grouping() const override { return "\3"; }
    };
    static std::locale locale{std::locale{}, new ThousandsSepPunct};
    return locale;
}

} // namespace

std::string formatCredits(uint64_t credits)
{
    if (credits == 0)
        return "-";
    return std::format(thousandsSepLocale(), "{:L}", credits);
}
