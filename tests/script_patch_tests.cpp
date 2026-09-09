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

    auto actor = bytes_of("function actor_binder:net_spawn(data)\r\n printf(\"actor net spawn\")\r\nend");
    const auto actor_size = actor.size();
    if (!ild::script_patch::bind_gameplay(actor) || actor.size() != actor_size ||
        string_of(actor).find("ild_gameplay.install()") == std::string::npos) return 8;
    auto ambiguous_actor = bytes_of("printf(\"actor net spawn\") printf(\"actor net spawn\")");
    if (ild::script_patch::bind_gameplay(ambiguous_actor)) return 9;
    // The repaired save branch must write the unmodified mod's ordinary bytes: a zero flag and no timestamp.
    auto save = bytes_of("\t\tpacker:w_bool(true)\r\n\t\tutils.w_CTime(packet, self.st.disable_input_time)\r\n"
        "printf(\"actor net spawn\")\r\n\tif stored_input_time == true then\r\n");
    const auto save_size = save.size();
    if (!ild::script_patch::bind_gameplay(save) || save.size() != save_size) return 10;
    const auto repaired = string_of(save);
    if (repaired.find("packer") != std::string::npos || repaired.find("w_CTime") != std::string::npos ||
        repaired.find("\t\tpacket:w_u8(0)     \r\n\t\t" + std::string(49, ' ') + "\r\n") == std::string::npos ||
        repaired.find("stored_input_time == 1    then") == std::string::npos) return 11;

    const std::string abort_source =
        "function abort(fmt, ...)\r\n"
        "\tlocal reason = string.format(fmt, ...)\r\n"
        "\tassert(\"ERROR: \" .. reason)\r\n"
        "\tprintf(\"ERROR: \" .. reason)\r\n"
        "\tprintf(\"%s\")\r\n"
        "end\r\n";
    auto abort_bytes = bytes_of(abort_source);
    const auto abort_size = abort_bytes.size();
    if (!ild::script_patch::reveal_abort_reason(abort_bytes) || abort_bytes.size() != abort_size) return 12;
    const auto revealed = string_of(abort_bytes);
    // The reason must reach the log without a second format pass, and the fatal must still be raised.
    if (revealed.find("printf") != std::string::npos || revealed.find("assert") != std::string::npos ||
        revealed.find("\tlog(\"ERROR: \" .. reason)   \r\n") == std::string::npos ||
        revealed.find("\terror(\"ERROR: \"..reason,2) \r\n") == std::string::npos ||
        std::count(revealed.begin(), revealed.end(), '\n') !=
            std::count(abort_source.begin(), abort_source.end(), '\n')) return 13;
    if (ild::script_patch::reveal_abort_reason(abort_bytes)) return 14;
    auto ambiguous_abort = bytes_of(abort_source + abort_source);
    if (ild::script_patch::reveal_abort_reason(ambiguous_abort)) return 15;

    return 0;
}
