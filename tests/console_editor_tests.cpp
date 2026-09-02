#include "console_editor.h"

#include <iostream>
#include <string>

int main()
{
    unsigned checks{};
    const auto check = [&checks](bool condition, const char* message)
    {
        ++checks;
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; return false; }
        return true;
    };
    ild::ConsoleEditor editor;
    editor.assign("renderer renderer_r2");
    editor.select_all();
    if (!check(editor.copy_text() == "renderer renderer_r2", "copy selection")) return 1;
    if (!check(editor.preview(true) == "[renderer renderer_r2]|", "selection is display-only")) return 1;
    if (!check(editor.text() == "renderer renderer_r2", "preview preserves command")) return 1;
    editor.insert("hud_info on");
    if (!check(editor.text() == "hud_info on", "paste replaces selection")) return 1;
    editor.home(false);
    editor.insert("x ");
    if (!check(editor.text() == "x hud_info on", "insert at caret")) return 1;
    editor.move(-1, true);
    editor.move(-1, true);
    if (!check(editor.copy_text() == "x ", "shift selection")) return 1;
    editor.erase(false);
    if (!check(editor.text() == "hud_info on", "delete selection")) return 1;
    editor.end(false);
    editor.move(-1, true, true);
    if (!check(editor.copy_text() == "on", "word selection")) return 1;
    editor.insert("off\r\nquit\t");
    if (!check(editor.text() == "hud_info off  quit ", "paste cannot execute a second command")) return 1;
    editor.assign(std::string(4000, 'a'));
    editor.insert("overflow");
    if (!check(editor.text().size() == ild::ConsoleEditor::maximum_length, "bounded insertion")) return 1;
    editor.select_all();
    editor.insert("new");
    if (!check(editor.text() == "new", "full buffer selection replacement")) return 1;
    editor.home(false);
    editor.erase(true);
    if (!check(editor.text() == "new", "backspace at beginning")) return 1;
    editor.end(false);
    editor.erase(false);
    if (!check(editor.text() == "new", "delete at end")) return 1;
    editor.sync("history command");
    if (!check(editor.cursor() == editor.text().size(), "native history resets caret")) return 1;
    std::cout << "PASS " << checks << " console-editor checks\n";
    return 0;
}
