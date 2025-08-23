#pragma once

#include "utf8_util.h"

#include <charconv>
#include <memory>
#include <span>
#include <vector>

namespace detail
{

class OptionBase
{
public:
    OptionBase(char nameShort, std::string_view nameLong)
        : nameShort(nameShort)
        , nameLong(nameLong)
    {
    }
    virtual ~OptionBase() = default;

    const char nameShort;
    std::string nameLong;

    virtual void setValue(std::string_view value) = 0;
};

template<typename T>
class Option : public OptionBase
{
public:
    Option(T &value, char nameShort, std::string_view nameLong)
        : OptionBase(nameShort, nameLong)
        , m_value(value)
    {
    }

    void setValue(std::string_view value) override
    {
        if constexpr (std::is_arithmetic_v<T>)
        {
            std::from_chars(value.begin(), value.end(), m_value);
        }
        else if constexpr (std::is_same_v<std::u32string, T>)
        {
            m_value = decodeUtf8(value);
        }
        else
        {
            m_value = value;
        }
    }

private:
    T &m_value;
};

} // namespace detail

class ArgParser
{
public:
    template<typename T>
    void addOption(T &value, char optionShort, std::string_view optionLong)
    {
        m_options.push_back(std::make_unique<detail::Option<T>>(value, optionShort, optionLong));
    }

    // Returns unparsed args
    std::vector<const char *> parse(std::span<const char *> args) const;

private:
    std::vector<std::unique_ptr<detail::OptionBase>> m_options;
};
