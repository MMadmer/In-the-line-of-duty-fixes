#include "script_patch.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

namespace
{
[[nodiscard]] std::vector<std::byte> bytes_of(const std::string& value)
{
    const auto* begin = reinterpret_cast<const std::byte*>(value.data());
    return {begin, begin + value.size()};
}

[[nodiscard]] std::string string_of(const std::vector<std::byte>& value)
{
    return {reinterpret_cast<const char*>(value.data()), value.size()};
}
}

int main()
{
    const std::string source =
        "function printf(fmt,...)\r\n"
        "get_console():execute(string.gsub(fmt, \" \", \"_\"))\r\n"
        "\tlog(string.format(fmt,...))\r\n"
        "end\r\n";

    auto patched = bytes_of(source);
    const auto original_size = patched.size();
    if (ild::script_patch::remove_console_execution(patched) != ild::script_patch::Result::applied ||
        patched.size() != original_size)
    {
        return 1;
    }

    const auto text = string_of(patched);
    if (text.find("get_console():execute") != std::string::npos || text.find("do end") == std::string::npos ||
        text.find("log(string.format(fmt,...))") == std::string::npos ||
        ild::script_patch::remove_console_execution(patched) != ild::script_patch::Result::already_applied)
    {
        return 2;
    }

    auto unsupported = bytes_of("function printf(fmt, ...) log(fmt) end\n");
    if (ild::script_patch::remove_console_execution(unsupported) != ild::script_patch::Result::unsupported_source)
    {
        return 3;
    }

    auto unrelated = bytes_of("do end\nfunction printf(fmt) log(fmt) end");
    if (ild::script_patch::remove_console_execution(unrelated) != ild::script_patch::Result::unsupported_source)
        return 4;
    auto ambiguous = bytes_of(source + source);
    if (ild::script_patch::remove_console_execution(ambiguous) != ild::script_patch::Result::unsupported_source)
        return 5;
    auto menu = bytes_of("function main_menu:__init() super()\r\n self:InitControls()\r\nend");
    const auto menu_size = menu.size();
    if (!ild::script_patch::bind_update_menu(menu) || menu.size() != menu_size ||
        string_of(menu).find("ild_fix_ui.i(self)") == std::string::npos)
        return 6;
    auto ambiguous_menu = bytes_of("self:InitControls() self:InitControls()");
    if (ild::script_patch::bind_update_menu(ambiguous_menu)) return 7;

    return 0;
}
