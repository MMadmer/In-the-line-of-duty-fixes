#include "update_bridge.h"
#include "command_line.h"
#include "inventory_hooks.h"
#include "texture_aliases.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <string_view>

#ifdef ILD_CONSOLE_QA
namespace ild::qa
{
void install_capture(const std::filesystem::path& root);
void request_capture(const std::string& state);
}
#endif

namespace ild
{
namespace
{
// This minimal command ABI is pinned to the validated SoC executable.
class ConsoleCommand
{
public:
    virtual ~ConsoleCommand() = default;
    virtual void Execute(const char*) = 0;
    virtual void Status(char (&)[256]) = 0;
    virtual void Info(char (&text)[256]) { strcpy_s(text, "update bridge"); }
    virtual void Save(void*) {}

private:
    const char* name_{"ild_update"};
    bool enabled_{true};
    bool lowercase_{};
    bool empty_arguments_{true};
};
static_assert(sizeof(ConsoleCommand) == 12);

std::string unescape(std::string_view value)
{
    std::string result;
    result.reserve(value.size());
    for (std::size_t index = 0; index < value.size(); ++index)
    {
        const auto token = value.substr(index, 3);
        if (token == "%25") { result.push_back('%'); index += 2; }
        else if (token == "%0A") { result.push_back('\n'); index += 2; }
        else result.push_back(value[index]);
    }
    return result;
}

class UpdateCommand final : public ConsoleCommand
{
public:
    void configure(const std::filesystem::path& root)
    {
        runtime_ = root / L".ild-fixes" / L"runtime";
        // Each game process talks to its own helper through session-named files.
        const auto session = std::to_wstring(GetCurrentProcessId());
        status_name_ = L"status-" + session + L".txt";
        command_name_ = L"command-" + session + L".txt";
        qa_ = has_switch(command_line_tail(GetCommandLineW()), L"-qa_update");
        std::array<wchar_t, 8> value{};
        const auto length = GetEnvironmentVariableW(L"ILD_QA_UI_DOWNLOAD", value.data(), 8);
        qa_download_ = qa_ && length == 1 && value[0] == L'1';
#ifdef ILD_CONSOLE_QA
        if (qa_) qa::install_capture(root);
#endif
    }

    void Execute(const char* arguments) override
    {
        if (!arguments) return;
        const std::string_view action(arguments);
        if (action.starts_with("read "))
        {
            refresh();
            selected_ = action.substr(5, 64);
            return;
        }
        constexpr std::array allowed{"download", "apply", "dismiss", "dismiss_major", "disable_major", "open_major"};
        if (std::find(allowed.begin(), allowed.end(), action) != allowed.end())
        {
            write_atomic(command_name_.c_str(), "session=" + std::to_string(GetCurrentProcessId()) +
                "\naction=" + std::string(action) + "\n");
        }
#ifdef ILD_CONSOLE_QA
        else if (qa_ && action == "qa_ui_ready")
        {
            qa::request_capture(field("state"));
            write_atomic(L"ui-proof.txt", "ui=CUIScriptWnd\nstate=" + field("state") + "\n");
        }
        else if (qa_ && action == "qa_game_ready") write_atomic(L"game-proof.txt", "actor=ready\n");
        else if (qa_ && action == "qa_smoke_done")
            write_atomic(L"game-proof.txt", "actor=ready\nsoak=5\ncompleted=1\n");
        else if (qa_ && action == "qa_assets")
            write_atomic(L"asset-proof.txt", verify_texture_repairs() ?
                "aliases=5\nmodel-boundary=1\nbytes-unchanged=1\n" : "failed=1\n");
        else if (qa_ && action == "qa_menu_ready") write_atomic(L"menu-proof.txt", "menu=ready\n");
        else if (qa_ && action == "qa_menu_done") write_atomic(L"menu-proof.txt", "menu=ready\nsoak=5\ncompleted=1\n");
        else if (qa_ && action == "qa_save_write")
            write_atomic(L"save-proof.txt", "timed-input=written\ncompleted=1\n");
        else if (qa_ && action == "qa_save_read")
            write_atomic(L"save-proof.txt", "timed-input=not-persisted\ncompleted=1\n");
        else if (qa_ && action == "qa_ui_last_page") qa::request_capture("last-page");
        else if (qa_ && action == "qa_game_done")
            write_atomic(L"game-proof.txt", "actor=ready\nsoak=5\nknife-checks=6\ndescriptions=5\ncompleted=1\n");
        else if (qa_ && action.starts_with("qa_knife "))
        {
            unsigned id{};
            const auto number = action.substr(9);
            const auto parsed = std::from_chars(number.data(), number.data() + number.size(), id);
            if (parsed.ec == std::errc{} && parsed.ptr == number.data() + number.size())
                knife_result_ = qa_inventory_swap(id);
        }
        else if (qa_ && action.starts_with("qa_fail "))
            write_atomic(L"game-failure.txt", std::string(action.substr(8)));
#endif
    }

    void Status(char (&text)[256]) override
    {
        std::string value;
        if (selected_ == "clock") value = std::to_string(GetTickCount64());
        else if (selected_ == "qa_download") value = qa_download_ ? "1" : "0";
#ifdef ILD_CONSOLE_QA
        else if (selected_ == "knife_result") value = knife_result_;
        else if (selected_ == "qa_save_mode")
        {
            wchar_t mode[16]{};
            if (GetEnvironmentVariableW(L"ILD_QA_SAVE_MODE", mode, 16) < 16)
            {
                if (std::wcscmp(mode, L"write") == 0) value = "write";
                if (std::wcscmp(mode, L"read") == 0) value = "read";
            }
        }
#endif
        else if (selected_.starts_with("changes:"))
        {
            const auto index_text = std::string_view(selected_).substr(8);
            unsigned index{};
            const auto parsed = std::from_chars(index_text.data(), index_text.data() + index_text.size(), index);
            if (parsed.ec == std::errc{} && parsed.ptr == index_text.data() + index_text.size() && index < 128)
            {
                const auto changes = field("changes");
                const auto offset = static_cast<std::size_t>(index) * 240;
                if (offset < changes.size()) value = changes.substr(offset, 240);
            }
        }
        else value = field(selected_);
        const auto length = (std::min)(value.size(), std::size_t{255});
        std::memcpy(text, value.data(), length);
        text[length] = 0;
    }

private:
    std::string field(const std::string& key) const
    {
        const auto found = fields_.find(key);
        return found == fields_.end() ? std::string{} : found->second;
    }

    bool read_status(std::string& data) const
    {
        // Shared delete access lets the helper replace the file while the menu is reading it.
        const auto file = CreateFileW((runtime_ / status_name_).c_str(), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        LARGE_INTEGER size{};
        DWORD read{};
        const auto complete = GetFileSizeEx(file, &size) && size.QuadPart >= 0 && size.QuadPart <= 65536 &&
            (data.assign(static_cast<std::size_t>(size.QuadPart), '\0'), true) &&
            ReadFile(file, data.data(), static_cast<DWORD>(data.size()), &read, nullptr) && read == data.size();
        CloseHandle(file);
        return complete;
    }

    void refresh()
    {
        const auto now = GetTickCount64();
        if (checked_ && now - last_read_ < 200) return;
        checked_ = true;
        last_read_ = now;
        std::string data;
        if (!read_status(data)) { fields_.clear(); return; }
        std::map<std::string, std::string> parsed;
        std::istringstream lines(data);
        std::string line;
        while (std::getline(lines, line) && parsed.size() < 64)
        {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const auto delimiter = line.find('=');
            if (delimiter != std::string::npos && delimiter < 64)
                parsed.emplace(line.substr(0, delimiter), unescape(std::string_view(line).substr(delimiter + 1)));
        }
        const auto session = parsed.find("session");
        if (session == parsed.end() || session->second != std::to_string(GetCurrentProcessId()))
        {
            fields_.clear();
            return;
        }
        fields_ = std::move(parsed);
    }

    void write_atomic(const wchar_t* name, const std::string& data)
    {
        std::error_code error;
        std::filesystem::create_directories(runtime_, error);
        if (error) return;
        const auto attributes = GetFileAttributesW(runtime_.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return;
        const auto target = runtime_ / name;
        const auto temporary = runtime_ / (std::wstring(name) + L".native-tmp");
        const auto file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return;
        DWORD written{};
        const auto complete = WriteFile(file, data.data(), static_cast<DWORD>(data.size()), &written, nullptr) &&
            written == data.size() && FlushFileBuffers(file);
        CloseHandle(file);
        if (complete)
            MoveFileExW(temporary.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
        else DeleteFileW(temporary.c_str());
    }

    std::filesystem::path runtime_;
    std::wstring status_name_, command_name_;
    std::map<std::string, std::string> fields_;
    std::string selected_;
    ULONGLONG last_read_{};
    bool checked_{}, qa_{}, qa_download_{};
#ifdef ILD_CONSOLE_QA
    std::string knife_result_;
#endif
};
}

bool install_update_bridge(HMODULE engine, const std::filesystem::path& root)
{
    using AddCommand = void(__thiscall*)(void*, ConsoleCommand*);
    const auto console = reinterpret_cast<void**>(GetProcAddress(engine, "?Console@@3PAVCConsole@@A"));
    const auto add = reinterpret_cast<AddCommand>(
        GetProcAddress(engine, "?AddCommand@CConsole@@QAEXPAVIConsole_Command@@@Z"));
    if (!console || !*console || !add) return false;
    static UpdateCommand command;
    command.configure(root);
    add(*console, &command);
    return true;
}
}
