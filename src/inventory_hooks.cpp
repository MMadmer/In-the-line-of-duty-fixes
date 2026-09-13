#include "inventory_hooks.h"
#include "iat_hook.h"
#include "item_descriptions.h"
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
// xrCore's shared_str is one pointer to a counted entry whose characters follow three 32-bit fields.
struct SharedName
{
    const unsigned char* entry;
};

using ToSlot = bool(__stdcall*)(void*, void*, bool);
using ReadString = const char*(__thiscall*)(void*, const char*, const char*);
using ReadSharedString = const char*(__thiscall*)(void*, const SharedName*, const char*);
using ReadUnsigned = unsigned(__thiscall*)(void*, const char*, const char*);
using ReadSharedUnsigned = unsigned(__thiscall*)(void*, const SharedName*, const char*);
using ReadFloat = float(__thiscall*)(void*, const char*, const char*);
using ReadSharedFloat = float(__thiscall*)(void*, const SharedName*, const char*);
using LineExists = int(__thiscall*)(void*, const char*, const char*);
X86Detour slot_hook;
ToSlot original_to_slot{};
ReadString original_read_string{};
ReadSharedString original_read_shared_string{};
ReadUnsigned original_read_unsigned{};
ReadSharedUnsigned original_read_shared_unsigned{};
ReadFloat original_read_float{};
ReadSharedFloat original_read_shared_float{};
LineExists original_line_exists{};
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

bool slot(void* inventory, void* item, bool activate)
{
    const auto target = game_base + 0x204A90;
    const unsigned not_activate = activate ? 0 : 1;
    unsigned char result{};
    __asm {
        mov ecx, inventory
        mov eax, item
        push not_activate
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

// CUIInventoryWnd::GetSlotList: the slot in ECX, the window in EDX; a slot the window draws no list for gives null.
void* slot_list(void* window, unsigned index)
{
    const auto target = game_base + 0x3BC590;
    void* result{};
    __asm {
        mov ecx, index
        mov edx, window
        call target
        mov result, eax
    }
    return result;
}

bool __stdcall to_slot(void* window, void* cell, bool force)
{
    if (!window || !cell) return false;
    const auto item = field<void*>(cell, 380);
    if (!item) return false;
    using GetSlot = unsigned(__thiscall*)(void*);
    const auto get_slot = reinterpret_cast<GetSlot>(field<void**>(item, 0)[41]);
    const auto index = get_slot(item);
    // The window owns lists for the pistol, rifle and outfit slots only, and it leaves the grenade slot alone by
    // itself. Every other slot - knife, binoculars, bolt, PDA, detector, torch - it moves the item into and then
    // hands the cell to the list it does not have, which is the crash the mod's own torch text describes. Those
    // slots are served here the way the engine serves a pickup: the same inventory calls, the same events, and a
    // rebuild of the bag view instead of a list.
    if (index == 3 || index == static_cast<unsigned>(-1) || slot_list(window, index))
        return original_to_slot(window, cell, force);
    const auto inventory = field<void*>(window, 9896);
    if (!inventory || field<void*>(item, 136) != inventory) return false;
    // CInventory::m_slots, sixteen bytes a slot: a vtable, the item, then the persistent flag.
    const auto begin = field<unsigned char*>(inventory, 56);
    const auto end = field<unsigned char*>(inventory, 60);
    if (!begin || !end || index >= static_cast<unsigned>(end - begin) / 16) return false;
    const auto entry = begin + 16 * index;
    if (field<unsigned char>(entry, 8)) return false;
    const auto previous = field<void*>(entry, 4);
    if (previous == item || (previous && !force)) return false;
    // The knife, the binoculars and the bolt are held in the hands and are drawn on being slotted, as the window
    // does for a pistol. A torch, a PDA or a detector has no hands to be drawn into: it is placed, and nothing is
    // asked to become the active item.
    const auto hand = index == 0 || index == 4 || index == 5;

    if (previous && !ruck(inventory, previous)) return false;
    if (!slot(inventory, item, hand))
    {
        if (previous) static_cast<void>(slot(inventory, previous, hand));
        return false;
    }
    if (previous) send_event(previous, 0x3BB8C0);
    send_event(item, 0x3BB740);
    if (hand) send_event(item, 0x3BB6D0);
    field<unsigned char>(window, 100) = 1;
    return true;
}

// Stable storage is required because the engine may intern a returned string later.
const char* keep_description(std::string_view original, std::string repaired)
{
    static std::mutex mutex;
    static std::map<std::string, std::string> descriptions;
    const std::lock_guard lock(mutex);
    return descriptions.try_emplace(std::string(original), std::move(repaired)).first->second.c_str();
}

[[nodiscard]] const char* name_of(const SharedName* name)
{
    return name && name->entry ? reinterpret_cast<const char*>(name->entry + 12) : nullptr;
}

[[nodiscard]] const char* repaired_text(const char* section, const char* key, const char* value)
{
    if (!value || !section || !key) return value;
    // A line the mod never gave text to at all, or gave the wrong one; the replacement is a literal and needs no
    // stable storage.
    if (const auto supplied = supplied_item_text(section, key)) return supplied;
    if (const auto corrected = corrected_item_text(section, key, value)) return corrected;
    if (std::strcmp(key, "description") != 0) return value;
    if (auto repaired = repaired_description(section, value)) return keep_description(value, std::move(*repaired));
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
    return keep_description(text, std::string(text.substr(0, warning)));
}

const char* __fastcall read_string(void* ini, void*, const char* section, const char* key)
{
    return repaired_text(section, key, original_read_string(ini, section, key));
}

const char* __fastcall read_shared_string(void* ini, void*, const SharedName* section, const char* key)
{
    return repaired_text(name_of(section), key, original_read_shared_string(ini, section, key));
}

// Only a value with a correction passes through a float; every other one stays the integer the engine read.
[[nodiscard]] unsigned repaired_unsigned(const char* section, const char* key, unsigned value)
{
    const auto corrected = section && key ? corrected_item_number(section, key, static_cast<float>(value)) :
        std::optional<float>{};
    return corrected ? static_cast<unsigned>(*corrected) : value;
}

[[nodiscard]] float repaired_float(const char* section, const char* key, float value)
{
    return section && key ? corrected_item_number(section, key, value).value_or(value) : value;
}

unsigned __fastcall read_unsigned(void* ini, void*, const char* section, const char* key)
{
    return repaired_unsigned(section, key, original_read_unsigned(ini, section, key));
}

unsigned __fastcall read_shared_unsigned(void* ini, void*, const SharedName* section, const char* key)
{
    return repaired_unsigned(name_of(section), key, original_read_shared_unsigned(ini, section, key));
}

float __fastcall read_float(void* ini, void*, const char* section, const char* key)
{
    return repaired_float(section, key, original_read_float(ini, section, key));
}

float __fastcall read_shared_float(void* ini, void*, const SharedName* section, const char* key)
{
    return repaired_float(name_of(section), key, original_read_shared_float(ini, section, key));
}

int __fastcall line_exists(void* ini, void*, const char* section, const char* key)
{
    const auto exists = original_line_exists(ini, section, key);
    // Only a present binding line is looked at, so the countless other queries cost one comparison.
    if (!exists || !section || !key || !original_read_string || std::strcmp(key, "script_binding") != 0)
        return exists;
    const auto value = original_read_string(ini, section, key);
    return value && hidden_item_line(section, key, value) ? FALSE : exists;
}

// The original is published before the import slot changes, since any thread may call through it at once.
template<class Function> void hook_import(HMODULE game, HMODULE core, const char* symbol, Function& original,
    void* replacement)
{
    original = core ? reinterpret_cast<Function>(GetProcAddress(core, symbol)) : nullptr;
    if (original) static_cast<void>(replace_iat_import(game, "xrCore.dll", symbol, replacement));
}
}

bool install_inventory_hooks(const std::filesystem::path& root)
{
    if (checked) return enabled;
    const auto module = GetModuleHandleW(L"xrGame.dll");
    if (!module) return false;
    checked = true;
    // Item text and numbers arrive through imports resolved by name, so their repairs do not need the validated build.
    // Every getter of both overloads is taken, because the engine reads one key through several of them.
    const auto core = GetModuleHandleW(L"xrCore.dll");
    hook_import(module, core, "?r_string@CInifile@@QAEPBDPBD0@Z", original_read_string,
        reinterpret_cast<void*>(&read_string));
    hook_import(module, core, "?r_string@CInifile@@QAEPBDABVshared_str@@PBD@Z", original_read_shared_string,
        reinterpret_cast<void*>(&read_shared_string));
    hook_import(module, core, "?r_u32@CInifile@@QAEIPBD0@Z", original_read_unsigned,
        reinterpret_cast<void*>(&read_unsigned));
    hook_import(module, core, "?r_u32@CInifile@@QAEIABVshared_str@@PBD@Z", original_read_shared_unsigned,
        reinterpret_cast<void*>(&read_shared_unsigned));
    hook_import(module, core, "?r_float@CInifile@@QAEMPBD0@Z", original_read_float,
        reinterpret_cast<void*>(&read_float));
    hook_import(module, core, "?r_float@CInifile@@QAEMABVshared_str@@PBD@Z", original_read_shared_float,
        reinterpret_cast<void*>(&read_shared_float));
    hook_import(module, core, "?line_exist@CInifile@@QAEHPBD0@Z", original_line_exists,
        reinterpret_cast<void*>(&line_exists));
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
    // The bag view is empty while the window has never been opened, which is how QA runs. An item of a slot the
    // window draws no list for is reached through the rucksack itself: the detour reads nothing of a cell but the
    // item it carries, so a bare cell stands in for the one the view would have built.
    const auto inventory = field<void*>(window, 9896);
    // A VC8 vector starts with its iterator proxy: m_ruck keeps its pointers at 24 and 28, m_belt at 40 and 44,
    // m_slots at 56 and 60. A second torch lands on the belt, so both containers are searched.
    std::array<void*, 4> containers{field<void*>(inventory, 24), field<void*>(inventory, 28),
        field<void*>(inventory, 40), field<void*>(inventory, 44)};
    for (std::size_t half = 0; half < containers.size(); half += 2)
    for (auto slot_item = static_cast<void**>(containers[half]);
        slot_item && slot_item < static_cast<void**>(containers[half + 1]); ++slot_item)
    {
        const auto item = *slot_item;
        if (!item || field<unsigned short>(field<void*>(item, 212), 164) != id) continue;
        using GetSlot = unsigned(__thiscall*)(void*);
        const auto index = reinterpret_cast<GetSlot>(field<void**>(item, 0)[41])(item);
        if (index == 3 || index == static_cast<unsigned>(-1) || slot_list(window, index)) return "slot-has-a-list";
        std::array<unsigned char, 400> bare{};
        field<void*>(bare.data(), 380) = item;
        const auto before = field<unsigned char*>(inventory, 12) - field<unsigned char*>(inventory, 8);
        if (!to_slot(window, bare.data(), true)) return "to-slot-rejected";
        const auto after = field<unsigned char*>(inventory, 12) - field<unsigned char*>(inventory, 8);
        if (before != after) return "inventory-count-changed";
        const auto entry = field<unsigned char*>(inventory, 56) + 16 * index;
        return field<void*>(entry, 4) == item ? "ok-through-rucksack" : "wrong-slotted-item";
    }
    // Nothing matched: say where the item actually sits, so a wrong container offset is visible in the record.
    static std::string where;
    where = "item-not-in-bag";
    for (unsigned offset = 8; offset <= 48; offset += 4)
    {
        const auto begin = field<void**>(inventory, offset);
        const auto end = field<void**>(inventory, offset + 4);
        if (!begin || !end || end < begin || end - begin > 256) continue;
        for (auto entry = begin; entry < end; ++entry)
        {
            MEMORY_BASIC_INFORMATION region{};
            if (!*entry || !VirtualQuery(*entry, &region, sizeof region) || region.State != MEM_COMMIT) continue;
            const auto object = field<void*>(*entry, 212);
            if (!object || !VirtualQuery(object, &region, sizeof region) || region.State != MEM_COMMIT) continue;
            if (field<unsigned short>(object, 164) == id) where += " in-vector-at-" + std::to_string(offset);
        }
    }
    return where.c_str();
}
#endif
}
