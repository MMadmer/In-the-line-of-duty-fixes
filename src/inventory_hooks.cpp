#include "inventory_hooks.h"
#include "iat_hook.h"
#include "sha256.h"
#include "x86_detour.h"
#include <array>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <string_view>

namespace ild
{
namespace
{
using ToSlot = bool(__stdcall*)(void*, void*, bool);
using ReadString = const char*(__thiscall*)(void*, const char*, const char*);
X86Detour slot_hook;
ToSlot original_to_slot{};
ReadString original_read_string{};
unsigned char* game_base{};
bool checked{}, enabled{};

template<class T> T& field(void* object, std::size_t offset)
{
    return *reinterpret_cast<T*>(static_cast<unsigned char*>(object) + offset);
}

bool ruck(void* inventory, void* item)
{
    const auto target = game_base + 0x204D90;
    unsigned char result{};
    // The supported binary uses an ESI inventory receiver for this internal call.
    __asm {
        push esi
        mov esi, inventory
        push item
        call target
        mov result, al
        pop esi
    }
    return result != 0;
}

bool slot(void* inventory, void* item)
{
    const auto target = game_base + 0x204A90;
    unsigned char result{};
    __asm {
        mov ecx, inventory
        mov eax, item
        push 0
        call target
        mov result, al
    }
    return result != 0;
}

void send_event(void* item, std::size_t offset)
{
    const auto target = game_base + offset;
    __asm {
        push edi
        mov edi, item
        call target
        pop edi
    }
}

bool __stdcall to_slot(void* window, void* cell, bool force)
{
    if (!window || !cell) return false;
    const auto item = field<void*>(cell, 380);
    if (!item) return false;
    using GetSlot = unsigned(__thiscall*)(void*);
    const auto get_slot = reinterpret_cast<GetSlot>(field<void**>(item, 0)[41]);
    if (get_slot(item) != 0) return original_to_slot(window, cell, force);
    const auto inventory = field<void*>(window, 9896);
    if (!inventory || field<void*>(item, 136) != inventory) return false;
    const auto slots = field<void*>(inventory, 56);
    if (!slots || field<unsigned char>(slots, 8)) return false;
    const auto previous = field<void*>(slots, 4);
    if (previous == item || (previous && !force)) return false;

    // Knife slot zero has no UI list. Reuse engine moves/events, then rebuild its bag view.
    if (previous && !ruck(inventory, previous)) return false;
    if (!slot(inventory, item))
    {
        if (previous) static_cast<void>(slot(inventory, previous));
        return false;
    }
    if (previous) send_event(previous, 0x3BB8C0);
    send_event(item, 0x3BB740);
    send_event(item, 0x3BB6D0);
    field<unsigned char>(window, 100) = 1;
    return true;
}

const char* __fastcall read_string(void* ini, void*, const char* section, const char* key)
{
    const auto value = original_read_string(ini, section, key);
    if (!value || !section || !key || std::strcmp(key, "description") != 0) return value;
    constexpr std::array knives{"wpn_knife_6x2", "wpn_knife_6x4", "wpn_knife_nkvd", "wpn_knife_tip30", "wpn_knify"};
    bool knife{};
    for (const auto name : knives) if (std::strcmp(name, section) == 0) knife = true;
    if (!knife) return value;
    constexpr std::string_view marker = "%c[10,9000000000,5,0]";
    const std::string_view text(value);
    const auto warning = text.find(marker);
    if (warning == std::string_view::npos) return value;
    static const auto expected = [] {
        constexpr wchar_t words[] = L"ВНИМАНИЕ! При смене ножей чрез инвентарь игра вылетает, да.";
        std::array<char, 256> result{};
        WideCharToMultiByte(1251, 0, words, -1, result.data(), static_cast<int>(result.size()), nullptr, nullptr);
        return std::string(result.data());
    }();
    if (!text.substr(warning + marker.size()).starts_with(expected)) return value;
    static std::mutex mutex;
    static std::map<std::string, std::string> descriptions;
    const std::lock_guard lock(mutex);
    // Stable storage is required because the engine may intern the returned string later.
    return descriptions.try_emplace(std::string(text), text.substr(0, warning)).first->second.c_str();
}
}

bool install_inventory_hooks(const std::filesystem::path& root)
{
    if (checked) return enabled;
    const auto module = GetModuleHandleW(L"xrGame.dll");
    if (!module) return false;
    checked = true;
    Sha256 hash{};
    if (!sha256_file(root / L"bin" / L"xrGame.dll", hash)) return false;
    constexpr char digits[] = "0123456789ABCDEF";
    std::string actual;
    for (const auto value : hash)
    {
        const auto byte = std::to_integer<unsigned>(value);
        actual += digits[byte >> 4];
        actual += digits[byte & 15];
    }
    if (actual != "277B67FD6D21839A2F6C246EF57C8AD0C31079C0EAAAB179A8072D1B74A0284F") return false;
    game_base = reinterpret_cast<unsigned char*>(module);
    constexpr unsigned char expected[]{0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8};
    if (!slot_hook.prepare(game_base + 0x3BBF80, reinterpret_cast<void*>(&to_slot), expected)) return false;
    original_to_slot = reinterpret_cast<ToSlot>(slot_hook.original());
    if (!slot_hook.enable()) return false;
    original_read_string = reinterpret_cast<ReadString>(replace_iat_import(module, "xrCore.dll",
        "?r_string@CInifile@@QAEPBDPBD0@Z", reinterpret_cast<void*>(&read_string)));
    enabled = true;
    return true;
}

#ifdef ILD_CONSOLE_QA
const char* qa_inventory_swap(unsigned id)
{
    if (!enabled) return "hooks-not-installed";
    const auto window = *reinterpret_cast<void**>(game_base + 0x560634);
    if (!window) return "inventory-window-missing";
    const auto init = game_base + 0x3BBCB0;
    __asm {
        push edi
        push esi
        mov edi, window
        xor esi, esi
        call init
        pop esi
        pop edi
    }
    const auto actor_inventory = field<void*>(window, 9896);
    const auto active = field<void*>(field<void*>(actor_inventory, 56), 4);
    if (active && field<unsigned short>(field<void*>(active, 212), 164) == id) return "ok";
    const auto bag = field<void*>(window, 4264);
    const auto container = field<void*>(bag, 128);
    const auto count = field<unsigned>(container, 40);
    const auto get_cell = game_base + 0x419480;
    for (unsigned index = 0; index < count; ++index)
    {
        void* cell{};
        __asm {
            push edi
            push esi
            mov edi, bag
            mov esi, index
            call get_cell
            mov cell, eax
            pop esi
            pop edi
        }
        if (!cell) continue;
        const auto item = field<void*>(cell, 380);
        if (!item || field<unsigned short>(field<void*>(item, 212), 164) != id) continue;
        const auto inventory = field<void*>(window, 9896);
        const auto before = field<unsigned char*>(inventory, 12) - field<unsigned char*>(inventory, 8);
        const auto entry = reinterpret_cast<ToSlot>(game_base + 0x3BBF80);
        if (!entry(window, cell, true)) return "to-slot-rejected";
        const auto after = field<unsigned char*>(inventory, 12) - field<unsigned char*>(inventory, 8);
        if (before != after) return "inventory-count-changed";
        const auto selected = field<void*>(field<void*>(inventory, 56), 4);
        if (selected != item) return "wrong-slotted-item";
        return "ok";
    }
    return "item-not-in-bag";
}
#endif
}
