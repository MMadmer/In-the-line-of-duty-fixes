#include "command_line.h"

#include <cstdlib>
#include <string_view>

using namespace std::literals;

namespace
{
constexpr bool tail_checks()
{
    return ild::command_line_tail(LR"("C:\Games\bin\XR_3DA.exe" -start server(all/single/alife/new) client(localhost) -nointro)"sv) ==
            LR"(-start server(all/single/alife/new) client(localhost) -nointro)"sv &&
        ild::command_line_tail(L"XR_3DA.exe"sv).empty() &&
        ild::command_line_tail(L"XR_3DA.exe   -qa_update"sv) == L"-qa_update"sv &&
        ild::command_line_tail(LR"("C:\Program Files\bin\XR_3DA.exe")"sv).empty() &&
        ild::command_line_tail(L"\"unterminated"sv).empty() &&
        ild::command_line_tail(LR"(XR_3DA.exe -fsltx "D:\my games\fsgame.ltx")"sv) == LR"(-fsltx "D:\my games\fsgame.ltx")"sv;
}

constexpr bool switch_checks()
{
    return ild::has_switch(L"-start server(x) -qa_update"sv, L"-qa_update"sv) &&
        !ild::has_switch(LR"(-fsltx "D:\my-qa_update\fsgame.ltx")"sv, L"-qa_update"sv) &&
        !ild::has_switch(L"-qa_update_more"sv, L"-qa_update"sv) &&
        ild::has_switch(L"\t-qa_update\t-nointro"sv, L"-qa_update"sv) &&
        !ild::has_switch(L""sv, L"-qa_update"sv);
}

static_assert(tail_checks());
static_assert(switch_checks());
}

int main()
{
    return tail_checks() && switch_checks() ? EXIT_SUCCESS : EXIT_FAILURE;
}
