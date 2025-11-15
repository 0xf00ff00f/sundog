#include "arg_parser.h"

#include <cstring>

std::vector<const char *> ArgParser::parse(std::span<const char *> args) const
{
    std::vector<const char *> unused;
    size_t index = 0;
    while (index < args.size())
    {
        const char *arg = args[index];
        bool parsed = false;
        for (auto &option : m_options)
        {
            if (arg[0] == '-')
            {
                const auto nameShort = option->nameShort;
                if (arg[1] == nameShort && arg[2] == '\0' && index < args.size() - 1)
                {
                    option->setValue(args[++index]);
                    parsed = true;
                    break;
                }
                else if (arg[1] == '-')
                {
                    const auto &nameLong = option->nameLong;
                    if (std::strcmp(arg + 2, nameLong.data()) == 0 && index < args.size() - 1)
                    {
                        option->setValue(args[++index]);
                        parsed = true;
                        break;
                    }
                    if (std::strncmp(arg + 2, nameLong.data(), nameLong.size()) == 0 && arg[2 + nameLong.size()] == '=')
                    {
                        option->setValue(arg + 3 + nameLong.size());
                        parsed = true;
                        break;
                    }
                }
            }
        }
        if (!parsed)
            unused.push_back(args[index]);
        ++index;
    }
    unused.insert(unused.end(), args.begin() + index, args.end());
    return unused;
}
