#pragma once

#include <string_view>

namespace ild
{
// X-Ray scans the raw command line for space-terminated switches such as "-start " and "-fsltx ", so a restart
// must reuse the original tail verbatim instead of re-quoting individual arguments.
[[nodiscard]] constexpr std::wstring_view command_line_tail(std::wstring_view line) noexcept
{
    constexpr auto npos = std::wstring_view::npos;
    std::size_t index = 0;
    if (!line.empty() && line.front() == L'"')
    {
        const auto closing = line.find(L'"', 1);
        index = closing == npos ? line.size() : closing + 1;
    }
    else
    {
        index = line.find_first_of(L" \t");
        if (index == npos) index = line.size();
    }
    while (index < line.size() && (line[index] == L' ' || line[index] == L'\t')) ++index;
    return line.substr(index);
}

// Whole-token match only: the switch inside a path or as a prefix of another switch does not count.
[[nodiscard]] constexpr bool has_switch(std::wstring_view arguments, std::wstring_view name) noexcept
{
    constexpr auto npos = std::wstring_view::npos;
    std::size_t index = 0;
    while (index < arguments.size())
    {
        while (index < arguments.size() && (arguments[index] == L' ' || arguments[index] == L'\t')) ++index;
        if (index >= arguments.size()) break;
        std::size_t end = 0;
        if (arguments[index] == L'"')
        {
            const auto closing = arguments.find(L'"', index + 1);
            end = closing == npos ? arguments.size() : closing + 1;
        }
        else
        {
            end = arguments.find_first_of(L" \t", index);
            if (end == npos) end = arguments.size();
        }
        if (arguments.substr(index, end - index) == name) return true;
        index = end;
    }
    return false;
}
}
