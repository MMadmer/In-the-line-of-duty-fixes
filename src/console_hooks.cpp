#include "console_hooks.h"
#include "console_editor.h"
#include "console_clipboard.h"
#include "x86_detour.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#ifdef ILD_CONSOLE_QA
#include <fstream>
#endif

namespace ild
{
namespace
{
using Execute = void(__thiscall*)(void*, const char*);
using Press = void(__thiscall*)(void*, int, int);
using Release = void(__thiscall*)(void*, int);
using Render = void(__thiscall*)(void*);
using Show = void(__thiscall*)(void*);

std::array<X86Detour, 5> detours;
Execute original_execute{};
Press original_press{};
Release original_release{};
Render original_render{};
Show original_show{};
ConsoleEditor editor;
bool left_control{}, right_control{}, left_shift{}, right_shift{};

char* input(void* console) { return static_cast<char*>(console) + 56; }
int& field(void* console, std::size_t offset) { return *reinterpret_cast<int*>(static_cast<char*>(console) + offset); }
void write_input(void* console, const std::string& value) { strcpy_s(input(console), 1024, value.c_str()); }

void __fastcall execute_hook(void* console, void*, const char* command)
{
    std::array<char, 1024> saved{};
    std::memcpy(saved.data(), input(console), saved.size());
    const std::array saved_fields{field(console, 28), field(console, 32), field(console, 48), field(console, 52)};
    original_execute(console, command);
    std::memcpy(input(console), saved.data(), saved.size());
    field(console, 28) = saved_fields[0];
    field(console, 32) = saved_fields[1];
    field(console, 48) = saved_fields[2];
    field(console, 52) = saved_fields[3];
}

void __fastcall show_hook(void* console, void*)
{
    if (!field(console, 1152))
    {
        left_control = right_control = left_shift = right_shift = false;
        editor.assign("");
    }
    original_show(console);
}

void __fastcall release_hook(void* console, void*, int key)
{
    if (key == 0x1D) left_control = false;
    if (key == 0x9D) right_control = false;
    if (key == 0x2A) left_shift = false;
    if (key == 0x36) right_shift = false;
    original_release(console, key);
    field(console, 40) = left_shift || right_shift;
}

void __fastcall press_hook(void* console, void*, int key, int repeated)
{
    editor.sync(input(console));
    if (key == 0x1D) left_control = true;
    if (key == 0x9D) right_control = true;
    if (key == 0x2A) left_shift = true;
    if (key == 0x36) right_shift = true;
    const auto control = left_control || right_control;
    const auto shift = left_shift || right_shift;
    bool handled = true;

    if (key == 0x1D || key == 0x9D || key == 0x2A || key == 0x36)
    {
        field(console, 40) = shift;
    }
    else if (control && key == 0x1E) editor.select_all();
    else if (control && (key == 0x2E || key == 0xD2))
    {
        if (!repeated) static_cast<void>(copy_console_clipboard(editor.copy_text()));
    }
    else if (control && key == 0x2D)
    {
        if (!repeated && copy_console_clipboard(editor.copy_text()))
        {
            if (editor.selection().first == editor.selection().second) editor.select_all();
            editor.erase(false);
        }
    }
    else if ((control && key == 0x2F) || (key == 0xD2 && !control))
    {
        if (!repeated) editor.insert(paste_console_clipboard());
    }
    else if (key == 0xCB) editor.move(-1, shift, control);
    else if (key == 0xCD) editor.move(1, shift, control);
    else if (key == 0xC7) editor.home(shift);
    else if (key == 0xCF) editor.end(shift);
    else if (key == 0x0E || key == 0xD3)
    {
        if (control && editor.selection().first == editor.selection().second)
            editor.move(key == 0x0E ? -1 : 1, true, true);
        editor.erase(key == 0x0E);
    }
    else handled = false;

    if (handled)
    {
        write_input(console, editor.text());
        field(console, 44) = 0;
        field(console, 20) = 0;
        return;
    }
    if (control) return;

    if (key == 1 || key == 0x0F || key == 0x1C || key == 0xC8 || key == 0xD0 ||
        key == 0xC9 || key == 0xD1 || (key == 0x29 && !shift))
    {
        original_press(console, key, repeated);
        editor.assign(input(console));
        return;
    }

    // Let the engine translate its scan codes, then insert the result at our caret.
    input(console)[0] = 0;
    original_press(console, key, repeated);
    const std::string inserted(input(console));
    editor.insert(inserted);
    write_input(console, editor.text());
}

void __fastcall render_hook(void* render_this, void*)
{
    // pureRender is the second base; all other console entry points use the complete object.
    auto console = static_cast<char*>(render_this) - 4;
    editor.sync(input(console));
    std::array<char, 1024> saved{};
    std::memcpy(saved.data(), input(console), saved.size());
    const auto blink = field(console, 1080);
    write_input(console, editor.preview(blink != 0));
    field(console, 1080) = 0;
    original_render(render_this);
    field(console, 1080) = blink;
    std::memcpy(input(console), saved.data(), saved.size());
}
}

bool install_console_hooks(HMODULE engine)
{
    if (!engine) return false;
    auto base = reinterpret_cast<unsigned char*>(engine);
    const std::array<unsigned char, 5> execute_bytes{0x8B, 0x44, 0x24, 0x04, 0x56};
    const std::array<unsigned char, 5> press_bytes{0x83, 0xEC, 0x08, 0x8B, 0xC1};
    std::array<unsigned char, 8> release_bytes{0xF3, 0x0F, 0x10, 0x05};
    const auto constant = reinterpret_cast<std::uintptr_t>(base + 0xD8D4C);
    std::memcpy(release_bytes.data() + 4, &constant, 4);
    const std::array<unsigned char, 6> render_bytes{0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8};
    const std::array<unsigned char, 10> show_bytes{0x56, 0x8B, 0xF1, 0x83, 0xBE, 0x80, 0x04, 0, 0, 0};

    // These whole-instruction prologues are pinned to the SHA-256-validated SoC executable.
    if (!detours[0].prepare(base + 0xB9A30, reinterpret_cast<void*>(&execute_hook), execute_bytes) ||
        !detours[1].prepare(base + 0xB8740, reinterpret_cast<void*>(&press_hook), press_bytes) ||
        !detours[2].prepare(base + 0xB95C0, reinterpret_cast<void*>(&release_hook), release_bytes) ||
        !detours[3].prepare(base + 0xB83A0, reinterpret_cast<void*>(&render_hook), render_bytes) ||
        !detours[4].prepare(base + 0xB98B0, reinterpret_cast<void*>(&show_hook), show_bytes)) return false;
    original_execute = reinterpret_cast<Execute>(detours[0].original());
    original_press = reinterpret_cast<Press>(detours[1].original());
    original_release = reinterpret_cast<Release>(detours[2].original());
    original_render = reinterpret_cast<Render>(detours[3].original());
    original_show = reinterpret_cast<Show>(detours[4].original());
    for (auto& detour : detours)
    {
        if (!detour.enable())
        {
            for (auto& installed : detours) installed.disable();
            return false;
        }
    }
    return true;
}

#ifdef ILD_CONSOLE_QA
void run_console_selftest(HMODULE engine, const std::filesystem::path& report)
{
    const auto console_pointer = reinterpret_cast<void**>(GetProcAddress(engine, "?Console@@3PAVCConsole@@A"));
    if (!console_pointer || !*console_pointer || !original_execute) return;
    auto console = *console_pointer;
    std::array<unsigned char, 1160> saved{};
    std::memcpy(saved.data(), console, saved.size());
    std::ofstream output(report);
    unsigned failed{};
    const auto check = [&output, &failed](bool pass, const char* name)
    {
        output << (pass ? "PASS " : "FAIL ") << name << '\n';
        if (!pass) ++failed;
    };
    editor.assign("hud_info on");
    write_input(console, editor.text());
    execute_hook(console, nullptr, "ild_console_qa_unknown_command");
    check(std::string(input(console)) == "hud_info on", "programmatic execution preserves input");
    press_hook(console, nullptr, 0x1D, 0);
    press_hook(console, nullptr, 0x1E, 0);
    release_hook(console, nullptr, 0x1D);
    check(editor.copy_text() == "hud_info on" && editor.selection().first == 0, "Ctrl+A selects input");
    press_hook(console, nullptr, 0x2D, 0);
    check(std::string(input(console)) == "x", "typed character replaces selection");
    editor.assign("abc");
    write_input(console, editor.text());
    press_hook(console, nullptr, 0xC7, 0);
    press_hook(console, nullptr, 0x2D, 0);
    check(std::string(input(console)) == "xabc", "native scan code inserts at caret");

    std::memcpy(console, saved.data(), saved.size());
    editor.assign(input(console));
    left_control = right_control = left_shift = right_shift = false;
    output << "failed=" << failed << '\n';
}
#endif
}
