#define DIRECTINPUT_VERSION 0x0800
#include "startup_repairs.h"
#include "audio_metadata.h"
#include "sha256.h"
#include "x86_detour.h"
#include "iat_hook.h"
#include "preset_compat.h"
#include <dinput.h>
#include <array>
#include <climits>
#include <cstdint>
#include <cstring>
#include <string>
#include <intrin.h>

namespace ild
{
namespace
{
X86Detour input_hook, comment_hook;
unsigned char* sound_base{};
unsigned char* engine_base{};
using ReadLine = void(__thiscall*)(void*, char*, unsigned);
ReadLine original_line{};

bool known_preset(void* reader)
{
    struct Prefix { void* vtable; const std::byte* data; int position; int size; };
    const auto& source = *static_cast<Prefix*>(reader);
    thread_local const std::byte* previous_data{};
    thread_local bool allowed{};
    if (!source.position || previous_data != source.data)
    {
        previous_data = source.data;
        allowed = false;
        if (source.size < 415 || source.size > 425 || !source.data) return false;
        Sha256 hash{};
        if (!sha256_bytes(std::span(source.data, static_cast<std::size_t>(source.size)), hash)) return false;
        constexpr char digits[] = "0123456789ABCDEF";
        std::string value;
        for (const auto byte : hash)
        {
            const auto part = std::to_integer<unsigned>(byte);
            value += digits[part >> 4];
            value += digits[part & 15];
        }
        constexpr std::array supported{
            "1FC4895EAEA286E639D7B02D9CD23B08AB010983C28467456B01C1A71A2BA02B",
            "5CA993E717BFD39F6053F2B9332E539B07D986890E556212BE7CE981EF5F4465",
            "26AF3802DAE8C655A99D60B4EB0B352C9F0243C94FFEDEFDE4041303FAFECFF1",
            "F15ED60F492481881935C1E45AD6BF80FF5AAAA1694DA86E3A2B0706AE8FE8AF",
            "28B0090831AB96CF691D316285DEBB33532FDEE0D1D0AB79494A440E91F0DA05",
            "90693B3443FB8E62EE84D987D902B60A2E2239731DDED02E864263C2570C44FE"};
        for (const auto expected : supported) if (value == expected) allowed = true;
    }
    return allowed;
}

void __fastcall preset_line(void* reader, void*, char* text, unsigned capacity)
{
    const auto adapt = _ReturnAddress() == engine_base + 0xB6545 && known_preset(reader);
    original_line(reader, text, capacity);
    // Migrate only the six stock presets: these obsolete settings have no command in the pinned release build.
    if (adapt && text && capacity && obsolete_preset_option(text)) text[0] = 0;
}

struct VorbisComment
{
    char** values;
    int* lengths;
    int count;
    char* vendor;
};
static_assert(sizeof(VorbisComment) == 16);
using GetComment = VorbisComment*(__cdecl*)(void*, int);
GetComment original_comment{};

struct Metadata
{
    std::uint32_t version{3};
    float minimum{}, maximum{}, volume{};
    std::uint32_t game_type{};
    float ai_maximum{};
};
static_assert(sizeof(Metadata) == 24);

bool __fastcall input_name(void* input, void*, int key, char* destination, std::size_t capacity)
{
    if (!destination || !capacity || capacity > INT_MAX) return false;
    destination[0] = 0;
    auto keyboard = *reinterpret_cast<IDirectInputDevice8W**>(static_cast<unsigned char*>(input) + 20);
    if (!keyboard) return false;
    DIPROPSTRING property{};
    property.diph.dwSize = sizeof(property);
    property.diph.dwHeaderSize = sizeof(property.diph);
    property.diph.dwObj = static_cast<DWORD>(key);
    property.diph.dwHow = DIPH_BYOFFSET;
    if (FAILED(keyboard->GetProperty(DIPROP_KEYNAME, &property.diph)) || !property.wsz[0]) return false;
    // SoC text assets use CP1251; the CRT's initial C locale cannot encode Cyrillic key names.
    return WideCharToMultiByte(1251, 0, property.wsz, -1, destination, static_cast<int>(capacity),
        nullptr, nullptr) > 0;
}

VorbisComment* __cdecl normalized_comment(void* source, void* decoder, int link, const void* caller)
{
    auto original = original_comment(decoder, link);
    if (caller != sound_base + 0xAC03 || !original || original->count < 0 || original->count > 4096)
        return original;
    if (original->count && (!original->values || !original->lengths)) return original;
    int selected = -1;
    bool malformed{};
    for (int index = 0; index < original->count; ++index)
    {
        if (original->lengths[index] < 0 || !original->values[index]) return original;
        const auto bytes = std::span(reinterpret_cast<const std::byte*>(original->values[index]),
            static_cast<std::size_t>(original->lengths[index]));
        const auto kind = classify_audio_comment(bytes);
        if (kind == AudioCommentKind::metadata) { selected = index; break; }
        malformed = malformed || kind == AudioCommentKind::malformed;
    }
    if (selected == 0 || (selected < 0 && malformed)) return original;

    thread_local Metadata metadata;
    thread_local char* data;
    thread_local int length;
    thread_local VorbisComment adapted;
    if (selected >= 0)
    {
        data = original->values[selected];
        length = original->lengths[selected];
    }
    else
    {
        // Fill absent X-Ray metadata from the source's existing defaults, preserving its audible/AI behavior.
        const auto bytes = static_cast<const unsigned char*>(source);
        std::memcpy(&metadata.volume, bytes + 32, 4);
        std::memcpy(&metadata.minimum, bytes + 36, 4);
        std::memcpy(&metadata.maximum, bytes + 40, 4);
        std::memcpy(&metadata.ai_maximum, bytes + 44, 4);
        std::memcpy(&metadata.game_type, bytes + 48, 4);
        data = reinterpret_cast<char*>(&metadata);
        length = sizeof(metadata);
    }
    adapted = {&data, &length, 1, original->vendor};
    return &adapted;
}

// LoadWave keeps its source in EBX; other Vorbis callers are returned unchanged.
__declspec(naked) VorbisComment* __cdecl comment_entry(void*, int)
{
    __asm {
        mov eax, [esp]
        push eax
        push dword ptr [esp+12]
        push dword ptr [esp+12]
        push ebx
        call normalized_comment
        add esp, 16
        ret
    }
}
}

bool install_input_name_fix(HMODULE engine)
{
    // The whole key-name query up to its GetProperty call. The prologue alone is shared by countless functions;
    // this is position independent, so only a build whose code is identical here is ever patched.
    constexpr std::array<unsigned char, 64> signature{
        0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x81, 0xEC, 0x1C, 0x02, 0x00, 0x00, 0x8B, 0x41, 0x14, 0x56,
        0x8B, 0x75, 0x08, 0x8D, 0x54, 0x24, 0x08, 0x52, 0xC7, 0x44, 0x24, 0x0C, 0x18, 0x02, 0x00, 0x00,
        0xC7, 0x44, 0x24, 0x10, 0x10, 0x00, 0x00, 0x00, 0x89, 0x74, 0x24, 0x14, 0xC7, 0x44, 0x24, 0x18,
        0x01, 0x00, 0x00, 0x00, 0x8B, 0x08, 0x6A, 0x14, 0x50, 0x8B, 0x41, 0x14, 0xFF, 0xD0, 0x85, 0xC0};
    const auto target = reinterpret_cast<unsigned char*>(engine) + 0x245D0;
    if (!engine || !code_matches(target, signature)) return false;
    return input_hook.prepare(target, reinterpret_cast<void*>(&input_name), std::span(signature).first(6)) &&
        input_hook.enable();
}

bool install_preset_compatibility(HMODULE engine)
{
    // The preset loader's own r_string call, whose return address preset_line recognises. On any other build that
    // address is not this call, so the adapter stays off rather than claiming to work.
    constexpr std::array<unsigned char, 17> call_site{
        0x68, 0x00, 0x04, 0x00, 0x00, 0x8D, 0x94, 0x24, 0x28, 0x04, 0x00, 0x00, 0x52, 0x8B, 0xC8, 0xFF, 0x15};
    if (!engine || !code_matches(reinterpret_cast<unsigned char*>(engine) + 0xB6530, call_site)) return false;
    engine_base = reinterpret_cast<unsigned char*>(engine);
    original_line = reinterpret_cast<ReadLine>(GetProcAddress(GetModuleHandleW(L"xrCore.dll"),
        "?r_string@IReader@@QAEXPADI@Z"));
    if (!original_line) return false;
    return replace_iat_import(engine, "xrCore.dll", "?r_string@IReader@@QAEXPADI@Z",
        reinterpret_cast<void*>(&preset_line));
}

bool install_audio_metadata_fix(const std::filesystem::path& root)
{
    static bool checked{}, installed{};
    if (checked) return installed;
    const auto module = GetModuleHandleW(L"xrSound.dll");
    if (!module) return false;
    checked = true;
    Sha256 hash{};
    if (!sha256_file(root / L"bin" / L"xrSound.dll", hash)) return false;
    constexpr char digits[] = "0123456789ABCDEF";
    std::string identity;
    for (const auto byte : hash)
    {
        const auto value = std::to_integer<unsigned>(byte);
        identity += digits[value >> 4];
        identity += digits[value & 15];
    }
    if (identity != "741FE39CDB2081CADB7CAEE33C111C60BE7EE1248F01FFB6B8F550AF50BCEFEA") return false;
    sound_base = reinterpret_cast<unsigned char*>(module);
    constexpr unsigned char expected[]{0x8B, 0x4C, 0x24, 0x04, 0x83, 0x79, 0x04, 0x00};
    if (!comment_hook.prepare(sound_base + 0xEF20, reinterpret_cast<void*>(&comment_entry), expected)) return false;
    original_comment = reinterpret_cast<GetComment>(comment_hook.original());
    installed = comment_hook.enable();
    return installed;
}
}
