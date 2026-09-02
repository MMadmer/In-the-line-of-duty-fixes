#include "script_patch.h"

#include <algorithm>
#include <array>
#include <string_view>

namespace ild::script_patch
{
namespace
{
constexpr std::string_view broken_statement = "get_console():execute(string.gsub(fmt, \" \", \"_\"))";
constexpr std::string_view replacement_prefix = "do end";
constexpr auto replacement_statement = []
{
    std::array<char, broken_statement.size()> value{};
    value.fill(' ');
    std::copy(replacement_prefix.begin(), replacement_prefix.end(), value.begin());
    return value;
}();

[[nodiscard]] auto as_bytes(std::string_view value)
{
    return std::as_bytes(std::span(value.data(), value.size()));
}
}

Result remove_console_execution(std::span<std::byte> source)
{
    const auto statement = as_bytes(broken_statement);
    const auto replacement = as_bytes(replacement_prefix);
    const auto match = std::search(source.begin(), source.end(), statement.begin(), statement.end());

    if (match == source.end())
    {
        const auto complete = std::as_bytes(std::span(replacement_statement));
        const auto safe = std::search(source.begin(), source.end(), complete.begin(), complete.end());
        return safe == source.end() ? Result::unsupported_source : Result::already_applied;
    }
    if (std::search(match + statement.size(), source.end(), statement.begin(), statement.end()) != source.end())
        return Result::unsupported_source;

    std::copy(replacement.begin(), replacement.end(), match);
    std::fill(match + replacement.size(), match + statement.size(), std::byte{' '});
    return Result::applied;
}

bool bind_update_menu(std::span<std::byte> source)
{
    const auto original = as_bytes("self:InitControls()");
    const auto replacement = as_bytes("ild_fix_ui.i(self)");
    const auto match = std::search(source.begin(), source.end(), original.begin(), original.end());
    if (match == source.end())
        return false;
    if (std::search(match + original.size(), source.end(), original.begin(), original.end()) != source.end())
        return false;
    std::copy(replacement.begin(), replacement.end(), match);
    std::fill(match + replacement.size(), match + original.size(), std::byte{' '});
    return true;
}

bool bind_gameplay(std::span<std::byte> source)
{
    const auto original = as_bytes("printf(\"actor net spawn\")");
    const auto replacement = as_bytes("ild_gameplay.install()");
    const auto match = std::search(source.begin(), source.end(), original.begin(), original.end());
    if (match == source.end() ||
        std::search(match + original.size(), source.end(), original.begin(), original.end()) != source.end()) return false;
    std::copy(replacement.begin(), replacement.end(), match);
    std::fill(match + replacement.size(), match + original.size(), std::byte{' '});
    const auto replace_equal = [&](std::string_view before, std::string_view after)
    {
        const auto old_bytes = as_bytes(before);
        const auto new_bytes = as_bytes(after);
        const auto at = std::search(source.begin(), source.end(), old_bytes.begin(), old_bytes.end());
        if (at != source.end() && old_bytes.size() == new_bytes.size())
            std::copy(new_bytes.begin(), new_bytes.end(), at);
    };
    replace_equal("packer:w_bool(true)", "packet:w_bool(true)");
    replace_equal("stored_input_time == true then", "stored_input_time == 1    then");
    return true;
}
}
