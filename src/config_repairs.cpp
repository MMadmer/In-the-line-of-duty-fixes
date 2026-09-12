#include "config_repairs.h"
#include "sha256.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <optional>

namespace ild
{
namespace
{
constexpr std::array sources{
    ConfigRepairSource{L"gamedata/config/gameplay/dialogs.xml", 142847,
        "2B4E97EBFB80940F6AFEE2468ACF9F6267474E3114C2055069CD17CF8B0EDE5D", ConfigRepair::mechanic_dialog},
    ConfigRepairSource{L"gamedata/config/gameplay/dialogs_escape.xml", 396468,
        "C8FC084D30FEDC7A626CDBBC1ABAD6A8C24E38CDAF0B5F8DB8534B57A1410712", ConfigRepair::prince_dialog},
    ConfigRepairSource{L"gamedata/config/text/rus/string_table_ui.xml", 11627,
        "FB930DEF8CE40C1D4EDFACE613A90744BD55A5D15C7A34F098602A7AA1FF23AB", ConfigRepair::skill_text},
    ConfigRepairSource{L"gamedata/config/gameplay/info_l03agroprom.xml", 38385,
        "FA0B5A8643EFB5D7EB946F7F217D4F718BD80CB1EB90D75A13067F3CCF6CC3A9", ConfigRepair::info_root},
    ConfigRepairSource{L"gamedata/config/scripts/kordon/esc_last_day_kamp.ltx", 584,
        "D729314649C63A44E0D2D6C4453E7668D6911E98A21E760AC7563B89F79550E4", ConfigRepair::camp_condition},
    ConfigRepairSource{L"gamedata/config/gameplay/dialogs_agroprom.xml", 76103,
        "DAC92CCD3085F46DCBD926D7238395A0B9658231ED043B50275B9D892104BDAE", ConfigRepair::burer_dialog},
    ConfigRepairSource{L"gamedata/config/scripts/kordon/z_zvyk_psi.ltx", 96,
        "B2D5E006DB9DF032E16DE14EF02B57CA7A1BE013257D3020D0FF9D12CC658678", ConfigRepair::psi_sound},
    ConfigRepairSource{L"gamedata/config/ui/avtodroch.xml", 7434,
        "B6FE2604675A392A2A7F4FDA1FC4B053D01557E1BEA504250199F1B9FE5D36FD", ConfigRepair::car_text},
    ConfigRepairSource{L"gamedata/config/gameplay/dialogs_bar.xml", 344431,
        "A1A1BCDAA2060B1B1223B8DE6BB780F56C9D985F88B16C61696339E645E2BDD6", ConfigRepair::skat_upgrade},
    ConfigRepairSource{L"gamedata/scripts/ogsm_mutants.script", 40094,
        "F00377127ACB3AA6EE474CA51F6FC59139BB71AD74B50968600FC51A6F24A2A3", ConfigRepair::mutant_sounds},
    ConfigRepairSource{L"gamedata/config/text/rus/string_table_tasks_escape.xml", 10767,
        "09B37C8A18C3183C5B2B87583BBA804162824B5AD65DF01660E78F4DE3F77810", ConfigRepair::vodka_task},
    ConfigRepairSource{L"gamedata/config/ui/maingame_16.xml", 3826,
        "6E80C195D264C4E431C59C0AB4A1782BC0EB2AEF1F753DAD25124BDDFF790C0B", ConfigRepair::counter_wide},
    ConfigRepairSource{L"gamedata/config/ui/maingame.xml", 3830,
        "E36AE22F91A0BDBBB61FD718686222B7C2D3DB8D4B2930F64D1A4B8DF2994454", ConfigRepair::counter_normal},
    ConfigRepairSource{L"gamedata/config/gameplay/dialogs_darkvalley.xml", 128073,
        "3614CFA556724034B31399AF2A8CEDC9E6643B1274B11F483F75014D7517D535", ConfigRepair::trader_refusal},
    ConfigRepairSource{L"gamedata/config/gameplay/character_desc_bar.xml", 114740,
        "E796FBC317FA9309E3697C06DBA484A045871E34DEDD5E05C4A8F7DC75620BA4", ConfigRepair::bronevik_profile},
    ConfigRepairSource{L"gamedata/config/gameplay/tasks_bar.xml", 21473,
        "6AE66AE92EB2520CE038E90CB9ED983D4CE8D9D0A56C78A16B490EE8001E9140", ConfigRepair::detector_task},
    ConfigRepairSource{L"gamedata/config/text/rus/stable_dialogs_bar.xml", 150407,
        "225271086B8F02FB8EC5E7D09C114087DAA7EFBC3BECCF22E02F8A920A5B9577", ConfigRepair::detector_text},
    ConfigRepairSource{L"gamedata/config/gameplay/character_desc_escape.xml", 211684,
        "F0E4CB80D6CCD7CB29D8D29DDE3F023CC88F1B0E6E5DA2E651B90F587C224B45", ConfigRepair::guide_shotgun},
    ConfigRepairSource{L"gamedata/config/gameplay/dialogs_yantar.xml", 31271,
        "E1AA3BD1C18D12E2A878497EF60F2EFA42333DE0464B43E5A4495EF47FF092EA", ConfigRepair::monolith_task},
    ConfigRepairSource{L"gamedata/config/gameplay/dialogs_garbage.xml", 71704,
        "204307CD9EDCE43F14EAF142026B820462247E2F6854619D5BB60F3A151FEA06", ConfigRepair::abram_pistol},
    ConfigRepairSource{L"gamedata/config/gameplay/tasks_darkvalley.xml", 12001,
        "3FE21D4E4BD4848157560C32A10BBDD87F22F841CB3DB77B9FDC4EAB9D1A75AB", ConfigRepair::samogon_task},
    ConfigRepairSource{L"gamedata/config/scripts/agr/agro_kotelnui_ybiica.ltx", 1608,
        "7D0DCC9BFE518A4DC45D02DBFAFDF67A4F2B36FAB47AD70EB46E0CCDE9E4F83D", ConfigRepair::killer_surrender},
    ConfigRepairSource{L"gamedata/config/text/rus/stable_dialogs_darkvalley.xml", 98716,
        "82AD412A73B50A1FDED9B5DB32619294D39EB829F44CCB4E3A6D20EE97960358", ConfigRepair::propysk_text},
    ConfigRepairSource{L"gamedata/config/text/rus/stable_storyline_info_escape.xml", 11546,
        "C7AF6D401B3AF40AE0EAF1605FD77410F7F85C0A6B4F3DF22426DC39B94FB140", ConfigRepair::skill_banner},
    ConfigRepairSource{L"gamedata/config/misc/quest_items.ltx", 149319,
        "9BFF52163E2AF65B4A3B1F99E75D1B7EB3CEF4747A17DF313767F16B11AF5740", ConfigRepair::yantar_sensor},
    ConfigRepairSource{L"gamedata/config/weapons/w_mp40.ltx", 8131,
        "5C5728E2B18581137A00F25B2022296E4DCC935D43F682998EBA29A5796A4BEA", ConfigRepair::german_smg_names},
    ConfigRepairSource{L"gamedata/config/weapons/w_b94.ltx", 4865,
        "B85845AB46F2F98958DCEF9A17EFDB5A6D5310316E435875C510774C40969196", ConfigRepair::b94_binding}
};

// Replace a unique expression with an equal-length one, padding the remainder with spaces.
bool blank_to(std::string& text, std::string_view old, std::string_view next)
{
    const auto at = text.find(old);
    if (at == text.npos || text.find(old, at + old.size()) != text.npos || next.size() > old.size()) return false;
    text.replace(at, next.size(), next);
    std::fill_n(text.begin() + at + next.size(), old.size() - next.size(), ' ');
    return true;
}

struct Range { std::size_t begin, end; };
std::optional<Range> block(std::string_view text, std::string_view tag, std::string_view id, std::size_t from = 0)
{
    const auto opening = "<" + std::string(tag) + " id=\"" + std::string(id) + "\">";
    const auto closing = "</" + std::string(tag) + ">";
    const auto begin = text.find(opening, from);
    if (begin == text.npos) return {};
    const auto end = text.find(closing, begin + opening.size());
    if (end == text.npos) return {};
    return Range{begin, end + closing.size()};
}

bool replace_one(std::string& text, std::string_view old, std::string_view next)
{
    const auto at = text.find(old);
    if (at == text.npos || text.find(old, at + old.size()) != text.npos || old.size() != next.size()) return false;
    text.replace(at, old.size(), next);
    return true;
}

bool remove_absent_action(std::string& text, std::string_view dialog, std::string_view action, unsigned expected)
{
    const auto owner = block(text, "dialog", dialog);
    if (!owner) return true;
    const auto token = "<action>" + std::string(action) + "</action>";
    unsigned count{};
    auto cursor = owner->begin;
    while ((cursor = text.find(token, cursor)) != text.npos && cursor < owner->end)
    {
        std::fill_n(text.begin() + cursor, token.size(), ' ');
        cursor += token.size();
        ++count;
    }
    return count == expected;
}

// Pay for inserted bytes with the block's own indentation. Whitespace between elements carries no meaning in
// this XML, so only the leading blanks of lines that start an element are shortened; one blank stays so
// neighbouring lines never fuse, and a line that carries text is never touched.
bool borrow_indentation(std::string& contents, std::size_t excess)
{
    std::string rebuilt;
    rebuilt.reserve(contents.size());
    std::size_t line{};
    while (line < contents.size())
    {
        auto end = contents.find('\n', line);
        end = end == contents.npos ? contents.size() : end + 1;
        auto text = std::string_view(contents).substr(line, end - line);
        const auto blanks = text.find_first_not_of(" \t");
        if (excess && blanks != text.npos && blanks >= 2 && text[blanks] == '<')
        {
            const auto removed = (std::min)(excess, blanks - 1);
            text.remove_prefix(removed);
            excess -= removed;
        }
        rebuilt.append(text);
        line = end;
    }
    if (excess) return false;
    contents = std::move(rebuilt);
    return true;
}

// A unique replacement that may change the length; the caller pays the difference back out of indentation.
bool swap_unique(std::string& text, std::string_view old, std::string_view next)
{
    const auto at = text.find(old);
    if (at == text.npos || text.find(old, at + old.size()) != text.npos) return false;
    text.replace(at, old.size(), next);
    return true;
}

// The mod repainted the HUD atlas with a round bezel, and the dial it paints for the contact counter is
// nowhere near where the stock plate used to be. The counter itself could not follow: it is laid out in
// maingame.xml inside the archives, so it still prints low and to the left of the dial. Moving the bezel
// would drag the whole minimap with it, so the counter moves instead, by what was measured between the
// number on screen and the centre of the dial in the atlas. Nothing is resized, so the file keeps its
// exact length.
constexpr std::string_view counter_wide_at = R"(<static_pda_online x="104" y="167")";
constexpr std::string_view counter_wide_fix = R"(<static_pda_online x="105" y="153")";
constexpr std::string_view counter_normal_at = R"(<static_pda_online x="138" y="167")";
constexpr std::string_view counter_normal_fix = R"(<static_pda_online x="135" y="148")";
static_assert(counter_wide_at.size() == counter_wide_fix.size());
static_assert(counter_normal_at.size() == counter_normal_fix.size());

// Bronevik looks the broken detector over for five thousand, names the EVA-1400 microchip it needs and says to
// bring it. The mod stops there: the chip is declared and spawned nowhere, no dialog takes it back and no
// repaired detector exists, so the money buys a sentence. The pack spawns the chip where the author meant to
// hide it and adds the one conversation that was missing. The topic is gated on holding both halves, so it
// appears only when the exchange can happen and closes itself once it has - no info portion of ours is needed.
constexpr std::string_view detector_dialog =
    "<dialog id=\"ild_bronevik_detektor_gotov\">"
    "<has_info>bar_bronevik_pochini_detektor</has_info>"
    "<precondition>ild_script_repairs.detector_parts_ready</precondition>"
    "<phrase_list>"
    "<phrase id=\"0\"><text>ild_bronevik_detektor_gotov_0</text><next>1</next></phrase>"
    "<phrase id=\"1\"><text>ild_bronevik_detektor_gotov_1</text>"
    "<action>ild_script_repairs.repair_detector</action></phrase>"
    "</phrase_list></dialog>";

// The two lines the dialog above names have to be entries in the string table, because the file that holds the
// dialog opens with a UTF-8 mark and is parsed as UTF-8: the mod's own CP1251 cannot be written into it. The
// engine builds that table from a fixed list of file names in the archived localization.ltx rather than from the
// language folder, so a string file of the pack's own is never opened - that is why every id of ours reached the
// player raw. They go where the Bar's other lines already are:
// - Броневик, я нашёл ту микросхему. EVA-1400, как ты и говорил.
// - Ну-ка... она самая, осколки целы. Давай сюда и детектор, посиди пока... Всё, готово, держи. Честно скажу -
//   такую машинку я в руках первый раз держу. Все аномалии на карте, как на ладони. Не урони больше в болото.
constexpr std::string_view detector_lines =
    "<string id=\"ild_bronevik_detektor_gotov_0\"><text>"
    "\xC1\xF0\xEE\xED\xE5\xE2\xE8\xEA\x2C\x20\xFF\x20\xED\xE0\xF8\xB8\xEB\x20\xF2\xF3\x20\xEC\xE8\xEA"
    "\xF0\xEE\xF1\xF5\xE5\xEC\xF3\x2E\x20\x45\x56\x41\x2D\x31\x34\x30\x30\x2C\x20\xEA\xE0\xEA\x20\xF2"
    "\xFB\x20\xE8\x20\xE3\xEE\xE2\xEE\xF0\xE8\xEB\x2E"
    "</text></string>"
    "<string id=\"ild_bronevik_detektor_gotov_1\"><text>"
    "\xCD\xF3\x2D\xEA\xE0\x2E\x2E\x2E\x20\xEE\xED\xE0\x20\xF1\xE0\xEC\xE0\xFF\x2C\x20\xEE\xF1\xEA\xEE"
    "\xEB\xEA\xE8\x20\xF6\xE5\xEB\xFB\x2E\x20\xC4\xE0\xE2\xE0\xE9\x20\xF1\xFE\xE4\xE0\x20\xE8\x20\xE4"
    "\xE5\xF2\xE5\xEA\xF2\xEE\xF0\x2C\x20\xEF\xEE\xF1\xE8\xE4\xE8\x20\xEF\xEE\xEA\xE0\x2E\x2E\x2E\x20"
    "\xC2\xF1\xB8\x2C\x20\xE3\xEE\xF2\xEE\xE2\xEE\x2C\x20\xE4\xE5\xF0\xE6\xE8\x2E\x20\xD7\xE5\xF1\xF2"
    "\xED\xEE\x20\xF1\xEA\xE0\xE6\xF3\x20\x2D\x20\xF2\xE0\xEA\xF3\xFE\x20\xEC\xE0\xF8\xE8\xED\xEA\xF3"
    "\x20\xFF\x20\xE2\x20\xF0\xF3\xEA\xE0\xF5\x20\xEF\xE5\xF0\xE2\xFB\xE9\x20\xF0\xE0\xE7\x20\xE4\xE5"
    "\xF0\xE6\xF3\x2E\x20\xC2\xF1\xE5\x20\xE0\xED\xEE\xEC\xE0\xEB\xE8\xE8\x20\xED\xE0\x20\xEA\xE0\xF0"
    "\xF2\xE5\x2C\x20\xEA\xE0\xEA\x20\xED\xE0\x20\xEB\xE0\xE4\xEE\xED\xE8\x2E\x20\xCD\xE5\x20\xF3\xF0"
    "\xEE\xED\xE8\x20\xE1\xEE\xEB\xFC\xF8\xE5\x20\xE2\x20\xE1\xEE\xEB\xEE\xF2\xEE\x2E"
    "</text></string>";

bool add_detector_lines(std::string& text)
{
    constexpr std::string_view anchor = "<string_table>";
    if (text.find("ild_bronevik_detektor_gotov_0") != text.npos) return false;
    const auto at = text.find(anchor);
    if (at == text.npos) return false;
    const auto size = text.size();
    text.insert(at + anchor.size(), detector_lines);
    return borrow_indentation(text, text.size() - size) && text.size() == size;
}

bool add_detector_dialog(std::string& text)
{
    if (text.find("ild_bronevik_detektor_gotov") != text.npos) return false;
    const auto owner = block(text, "dialog", "bar_bronevik_pochini_detektor");
    if (!owner) return true;
    const auto size = text.size();
    text.insert(owner->end, detector_dialog);
    return borrow_indentation(text, text.size() - size) && text.size() == size;
}

// The dialog reaches the player only from Bronevik's own topic list.
bool add_detector_topic(std::string& text)
{
    constexpr std::string_view anchor = "<actor_dialog>bar_bronevik_pochini_detektor</actor_dialog>";
    constexpr std::string_view added = "<actor_dialog>ild_bronevik_detektor_gotov</actor_dialog>\r\n\t\t";
    if (text.find("ild_bronevik_detektor_gotov") != text.npos) return false;
    const auto at = text.find(anchor);
    if (at == text.npos || text.find(anchor, at + anchor.size()) != text.npos) return false;
    const auto size = text.size();
    text.insert(at, added);
    return borrow_indentation(text, text.size() - size) && text.size() == size;
}

// The detector job had no PDA entry at all - the portion its conversation grants is declared empty and no task
// names it. This is the skeleton the engine needs to know the task exists; its two steps, and the map spots
// bound to them, are built in script where the cache and Bronevik can actually be found, because neither has a
// story id for the XML form to point at.
// The title carries its own CP1251 bytes for the same reason the dialog above does: Ремонт научного детектора.
constexpr std::string_view detector_task_entry =
    "<game_task id=\"ild_detector_task\"><title>"
    "\xD0\xE5\xEC\xEE\xED\xF2\x20\xED\xE0\xF3\xF7\xED\xEE\xE3\xEE\x20\xE4\xE5\xF2\xE5\xEA\xF2\xEE\xF0\xE0"
    "</title><objective><text>"
    "\xD0\xE5\xEC\xEE\xED\xF2\x20\xED\xE0\xF3\xF7\xED\xEE\xE3\xEE\x20\xE4\xE5\xF2\xE5\xEA\xF2\xEE\xF0\xE0"
    "</text><icon>ui_iconsTotal_artefact</icon></objective></game_task>";

bool add_detector_task(std::string& text)
{
    if (text.find("ild_detector_task") != text.npos) return false;
    const auto size = text.size();
    text.insert(0, detector_task_entry);
    return borrow_indentation(text, text.size() - size) && text.size() == size;
}

// The ransom dialog hands over the artefact case without the 250 000 roubles that its own lines, its task and
// the mod's check and charge functions all describe; those two functions are simply never referenced. The
// gate goes on the dialog, the charge on the phrase that hands the case over, both at the file's exact size.
bool add_ransom_check(std::string& text)
{
    // A source without the dialog has nothing to gate; only a dialog that is present and unpatched is changed.
    const auto owner = block(text, "dialog", "esc_dengi_rebe");
    if (!owner) return true;
    auto contents = text.substr(owner->begin, owner->end - owner->begin);
    const auto size = contents.size();
    constexpr std::string_view gate = "<dont_has_info>esc_dengi_rebe</dont_has_info>";
    constexpr std::string_view precondition = "<precondition>new_life.don_reba_denga_za_artu_esti</precondition>";
    constexpr std::string_view handover = "<action>new_life.sidor_mne_keis_s_artami</action>";
    constexpr std::string_view charge = "<action>new_life.ia_otdaq_rebe_dengy_250000</action>";
    if (contents.find("don_reba_denga_za_artu_esti") != contents.npos) return false;
    auto at = contents.find(gate);
    if (at == contents.npos || contents.find(gate, at + gate.size()) != contents.npos) return false;
    contents.insert(at + gate.size(), precondition);
    at = contents.find(handover);
    if (at == contents.npos || contents.find(handover, at + handover.size()) != contents.npos) return false;
    contents.insert(at, charge);
    if (!borrow_indentation(contents, contents.size() - size) || contents.size() != size) return false;
    text.replace(owner->begin, size, contents);
    return true;
}

// Tikhon's closing line hands over "your share" - the quarter million he guarantees for the ATP deal, and the sum
// Reba names for the case - and the dialog has no action at all, so nothing was ever paid. The share is paid on
// that very line by the pack's script. It rides on the same repair as the ransom's charge, so an installation
// never gets one without the other: where the ransom gate went in, the share must go in too.
bool add_tikhon_share(std::string& text, bool required)
{
    const auto owner = block(text, "dialog", "esc_tixon_atp_final");
    if (!owner) return !required;
    auto contents = text.substr(owner->begin, owner->end - owner->begin);
    const auto size = contents.size();
    constexpr std::string_view line = "<text>esc_tixon_atp_final_1</text>";
    constexpr std::string_view share = "<action>ild_script_repairs.tikhon_share</action>";
    if (contents.find("tikhon_share") != contents.npos) return false;
    const auto at = contents.find(line);
    if (at == contents.npos || contents.find(line, at + line.size()) != contents.npos) return false;
    contents.insert(at + line.size(), share);
    if (!borrow_indentation(contents, contents.size() - size) || contents.size() != size) return false;
    text.replace(owner->begin, size, contents);
    return true;
}

// An offer whose only reply for a player who cannot pay records a refusal that the dialog itself is gated on, and
// nothing ever clears it: one visit without money removes the offer for good. The one-shot the author meant is the
// paid branch, which closes the dialog through its own portion, so the refusal gate and its only writer go.
bool reopen_refused_offer(std::string& text, std::string_view dialog, std::string_view refusal)
{
    const auto owner = block(text, "dialog", dialog);
    if (!owner) return false;
    auto contents = text.substr(owner->begin, owner->end - owner->begin);
    if (!blank_to(contents, "<dont_has_info>" + std::string(refusal) + "</dont_has_info>", "") ||
        !blank_to(contents, "<give_info>" + std::string(refusal) + "</give_info>", "")) return false;
    text.replace(owner->begin, contents.size(), contents);
    return true;
}
}

std::span<const ConfigRepairSource> config_repair_sources() { return sources; }

bool is_config_repair_path(std::wstring_view path, const std::filesystem::path& root)
{
    for (const auto& source : sources)
    {
        auto expected = (root / source.relative).wstring();
        for (auto& character : expected) if (character == L'/') character = L'\\';
        if (path.size() == expected.size() && _wcsnicmp(path.data(), expected.c_str(), path.size()) == 0) return true;
    }
    return false;
}

bool is_config_repair_size(std::size_t size)
{
    for (const auto& source : sources)
        if (source.size == size) return true;
    return false;
}

// A reply list that lost one of its entries. The branch it should name is written, gated and wired to logic;
// nothing else can reach it, so the phrases behind it are dead text. The missing next goes in beside the
// sibling it belongs with and is paid for out of the dialog's own indentation, the way the detector task entry
// is, so the file keeps its exact length.
bool add_missing_reply(std::string& text, std::string_view dialog, std::string_view phrase,
    std::string_view sibling, std::string_view added)
{
    const auto owner = block(text, "dialog", dialog);
    // A source without the dialog has no reply list to repair, the way an absent dialog has no action to
    // remove. Every file this runs on is pinned by size and digest, so absence means a fixture, not a release.
    if (!owner) return true;
    auto contents = text.substr(owner->begin, owner->end - owner->begin);
    const auto size = contents.size();
    const auto hub = block(contents, "phrase", phrase);
    if (!hub || contents.find(added) != contents.npos) return false;
    const auto at = contents.find(sibling, hub->begin);
    if (at == contents.npos || at >= hub->end ||
        contents.find(sibling, at + sibling.size()) < hub->end) return false;
    contents.insert(at + sibling.size(), added);
    if (!borrow_indentation(contents, contents.size() - size) || contents.size() != size) return false;
    text.replace(owner->begin, size, contents);
    return true;
}

// The moonshine job's header objective - the one the whole task completes on - was left with an empty
// infoportion_complete, so it could never match a known portion and the job stayed in the PDA for the rest of
// the game. Its own last step already names the portion that line grants; the header takes the same one, which
// is how every neighbouring task in this file is built.
bool close_samogon_task(std::string& text)
{
    constexpr std::string_view empty = "<infoportion_complete></infoportion_complete>";
    constexpr std::string_view portion = "val_kom_mazai_6";
    const auto owner = block(text, "game_task", "val_kvest_mazai_samogon");
    if (!owner) return false;
    auto contents = text.substr(owner->begin, owner->end - owner->begin);
    const auto size = contents.size();
    const auto at = contents.find(empty);
    if (at == contents.npos || contents.find(empty, at + empty.size()) != contents.npos) return false;
    contents.insert(at + empty.find('>') + 1, portion);
    if (!borrow_indentation(contents, contents.size() - size) || contents.size() != size) return false;
    text.replace(owner->begin, size, contents);
    return true;
}

// The Dark Valley toll gate. Its eight lines are filed under ids that carry an extra "chr_", while the dialog
// names them without it, so nothing resolves and the player reads the identifiers off the screen. The string
// ids are the side that moves: dropping four bytes is paid for with spaces on the same line, where renaming the
// dialog's own text would have added four bytes eight times.
bool rename_propysk_strings(std::string& text)
{
    for (char index = '0'; index <= '7'; ++index)
    {
        const std::string declared = std::string("<string id=\"val_chr_band_oxra_dai_propysk_") + index + "\">";
        const std::string named = std::string("<string id=\"val_band_oxra_dai_propysk_") + index + "\">";
        if (!blank_to(text, declared, named)) return false;
    }
    return true;
}

// The Bar string table declares bar_iahik_nac_2_1_0 twice, and the second block holds what the dialog calls
// bar_iahik_nac_2_1_1. The table keeps the last entry of a duplicate id, so the player's own topic line printed
// the courier's reply and the reply itself printed its id raw. Only the second declaration is renamed.
bool split_crate_reply(std::string& text)
{
    constexpr std::string_view declaration = "<string id=\"bar_iahik_nac_2_1_0\">";
    if (text.find("bar_iahik_nac_2_1_1") != text.npos) return false;
    const auto first = text.find(declaration);
    // A source that never declared it, or declared it once, has no duplicate to split - the same contract the
    // absent-action repair keeps. A third declaration is not the file this was written for.
    if (first == text.npos) return true;
    const auto second = text.find(declaration, first + declaration.size());
    if (second == text.npos) return true;
    if (text.find(declaration, second + declaration.size()) != text.npos) return false;
    text[second + declaration.size() - 3] = '1';
    return true;
}

// The guide who leaves with Yegor is spawned from a profile whose supplies name wpn_spas. No such section
// exists - the shotgun the mod ships is wpn_spas12 - and a trader's supplies are built inside the spawn call,
// before any script can see the object, so the missing section is an outright fatal. The name gains two
// characters; they are paid for out of the leading blanks of the line below, which the parser trims anyway.
constexpr std::string_view guide_shotgun_at = "\t\t\twpn_spas \\n\r\n       \t\t\tammo_9x39_pab9";
constexpr std::string_view guide_shotgun_fix = "\t\t\twpn_spas12 \\n\r\n     \t\t\tammo_9x39_pab9";
static_assert(guide_shotgun_at.size() == guide_shotgun_fix.size());

// The mechanic skill banners. The Cordon storyline file declares the level-1 id three times and the third block
// carries the level-2 caption, so the last-one-wins string table gave the level-1 banner the level-2 text, and
// the level-2 id was declared nowhere and reached the player raw. Only the block holding the level-2 caption is
// renamed: the caption is what identifies it.
constexpr std::string_view mechanic_level_two =
    "<string id=\"stalkerok_navuk_mex_lvl1\">\r\n\t\t<text>\""
    "\xD1\xD3\xCF\xC5\xD0-\xCC\xC5\xD5\xC0\xCD\xC8\xCA\"";
constexpr std::string_view mechanic_level_two_fixed =
    "<string id=\"stalkerok_navuk_mex_lvl2\">\r\n\t\t<text>\""
    "\xD1\xD3\xCF\xC5\xD0-\xCC\xC5\xD5\xC0\xCD\xC8\xCA\"";
static_assert(mechanic_level_two.size() == mechanic_level_two_fixed.size());

// The Yantar sensor stalkers carry on four levels lost both halves of its inventory entry: the grid keys hold a
// pixel position instead of a cell index, which the engine multiplies by fifty again, and the name was cut down
// to one letter no string table can translate. Vanilla's own cell comes back together with the string id the
// neighbouring short name already uses; the padding around the equals signs pays for it.
constexpr std::string_view yantar_sensor_at =
    "inv_name\t\t\t= i\r\n"
    "inv_name_short\t\t= item_detector_yantar_name\r\n"
    "inv_weight\t\t\t= 0\r\n"
    "\r\n"
    "inv_grid_width\t\t= 1\r\n"
    "inv_grid_height\t\t= 1\r\n"
    "inv_grid_x\t\t\t= 4000\r\n"
    "inv_grid_y\t\t\t= 1950\r\n"
    "cost\t\t\t\t= 30";
constexpr std::string_view yantar_sensor_fix =
    "inv_name= item_detector_yantar_name\r\n"
    "inv_name_short= item_detector_yantar_name\r\n"
    "inv_weight= 0\r\n"
    "\r\n"
    "inv_grid_width= 1\r\n"
    "inv_grid_height= 1\r\n"
    "inv_grid_x= 5\r\n"
    "inv_grid_y= 14\r\n"
    "cost= 30";
static_assert(yantar_sensor_fix.size() <= yantar_sensor_at.size());

// The two German submachine guns hold each other's name, and neither value is a string table id. The mod names
// its other war-era weapons with plain CP1251 text in this very field, and the file carries no byte order mark,
// so the names are written the same way. The MP-41 inherits from the MP-40 and never declared a short name of
// its own, so the line it needs is paid for out of the blanks around the following visual's equals sign.
constexpr std::string_view mp40_name_at = "inv_name\t\t\t\t= mp41\r\ninv_name_short\t\t\t= mp41";
constexpr std::string_view mp40_name_fix = "inv_name\t\t\t= \xCC\xCF-40\r\ninv_name_short\t\t= \xCC\xCF-40";
constexpr std::string_view mp41_name_at = "inv_name\t\t\t\t= mp40\r\n\r\nvisual                  = ";
constexpr std::string_view mp41_name_fix = "inv_name=\xCC\xCF-41\r\ninv_name_short=\xCC\xCF-41\r\n\r\nvisual= ";
static_assert(mp40_name_at.size() == mp40_name_fix.size());
static_assert(mp41_name_at.size() == mp41_name_fix.size());

bool repair_config_text(std::string& text, ConfigRepair repair)
{
    auto patched = text;
    if (repair == ConfigRepair::camp_condition)
    {
        if (!replace_one(patched, "active = {esc_delet_makar_i_tp} kamp2, kamp",
            "active ={+esc_delet_makar_i_tp} kamp2, kamp")) return false;
    }
    else if (repair == ConfigRepair::burer_dialog)
    {
        // Keep the existing final coin exchange; the earlier rejected-deal action has no implementation.
        if (!remove_absent_action(patched, "agro_zombi_petia_2", "moa_agro.ot_menia_monetu_bqrera", 1)) return false;
    }
    else if (repair == ConfigRepair::counter_wide || repair == ConfigRepair::counter_normal)
    {
        const auto wide = repair == ConfigRepair::counter_wide;
        if (!swap_unique(patched, wide ? counter_wide_at : counter_normal_at,
            wide ? counter_wide_fix : counter_normal_fix)) return false;
    }
    else if (repair == ConfigRepair::psi_sound)
    {
        if (!replace_one(patched, "on_use = no_use", ";n_use = no_use")) return false;
    }
    else if (repair == ConfigRepair::mutant_sounds)
    {
        // These two sound files ship in neither the loose tree nor the archives, and the constructors run at
        // module scope, so the module cannot even be loaded, let alone patched from Lua. Both use sites already
        // test the value before playing it, so a nil leaves the effect intact and merely silent.
        if (!blank_to(patched, "sound_object([[anomaly\\flies]])", "nil") ||
            !blank_to(patched, "sound_object([[monsters\\phantom\\phantom_snork_death]])", "nil")) return false;
    }
    else if (repair == ConfigRepair::skat_upgrade)
    {
        // The material-5 SKAT branch was cloned from the material-7 phrase and kept its reward, so the
        // material-5 outfit has no producer at all. Only this phrase is retargeted, never the material-7 one.
        constexpr std::string_view anchor = "b_bronevik_modern_505<";
        constexpr std::string_view reward = "pochinka.b_mne_mod_skat7";
        const auto phrase = patched.find(anchor);
        if (phrase == patched.npos || patched.find(anchor, phrase + anchor.size()) != patched.npos) return false;
        const auto action = patched.find(reward, phrase);
        if (action == patched.npos || action - phrase > 200) return false;
        patched[action + reward.size() - 1] = '5';
        // Bronevik's detector inspection: the only reply without 5000 roubles hid the offer for the rest of the game.
        if (!reopen_refused_offer(patched, "bar_bronevik_pochini_detektor", "bar_bronevik_ne_chini_detektor"))
            return false;
        // The wish-granter branch of the Bar mechanic's drinking talk answers and then names phrase 2 - the
        // one-shot "here is your vodka" reply, which is gated on still carrying a bottle. Every other branch of
        // the same dialog returns to the hub instead: 13 and 23 both name 4. The bottle is handed over at the
        // start of that very conversation, so by the time the answer is given phrase 2 is filtered out and the
        // branch has no reachable reply at all, with the talk window still open on it. The branch is pointed at
        // the hub its siblings use; one digit, so the file keeps its length.
        constexpr std::string_view wish_anchor = "b_nac_mex_bazar_17<";
        constexpr std::string_view wish_jump = "<next>2</next>";
        const auto wish = patched.find(wish_anchor);
        if (wish == patched.npos || patched.find(wish_anchor, wish + wish_anchor.size()) != patched.npos)
            return false;
        const auto jump = patched.find(wish_jump, wish);
        if (jump == patched.npos || jump - wish > 200) return false;
        patched[jump + wish_jump.find('2')] = '4';
        if (!add_detector_dialog(patched)) return false;
    }
    else if (repair == ConfigRepair::bronevik_profile)
    {
        if (!add_detector_topic(patched)) return false;
    }
    else if (repair == ConfigRepair::detector_task)
    {
        if (!add_detector_task(patched)) return false;
    }
    else if (repair == ConfigRepair::detector_text)
    {
        if (!split_crate_reply(patched) || !add_detector_lines(patched)) return false;
    }
    else if (repair == ConfigRepair::guide_shotgun)
    {
        if (!replace_one(patched, guide_shotgun_at, guide_shotgun_fix)) return false;
    }
    else if (repair == ConfigRepair::monolith_task)
    {
        // The professor's optional suit errand closes on its own portion and then grants a second one that
        // belongs to another quest entirely, dropping a permanently dead task into the PDA. Only that one grant
        // goes; the handover and the spawn that follows it are untouched.
        constexpr std::string_view closing = "<give_info>monolit_done</give_info>";
        constexpr std::string_view stray = "<give_info>yan_find_scientist_semenov_start</give_info>";
        const auto at = patched.find(closing);
        if (at == patched.npos || patched.find(closing, at + closing.size()) != patched.npos) return false;
        const auto grant = patched.find(stray, at);
        if (grant == patched.npos || grant - at > 80) return false;
        std::fill_n(patched.begin() + grant, stray.size(), ' ');
    }
    else if (repair == ConfigRepair::abram_pistol)
    {
        // Abram begs for a pistol and the mod writes both answers, numbering them in their own text, but the
        // phrase that asks names only the first. The second is the one the Garbage logic waits for.
        if (!add_missing_reply(patched, "gar_stalk_baraxol3_k_baraxolke", "6",
            "<next>7</next>", "<next>8</next>")) return false;
    }
    else if (repair == ConfigRepair::samogon_task)
    {
        if (!close_samogon_task(patched)) return false;
    }
    else if (repair == ConfigRepair::killer_surrender)
    {
        // The remark section carries on_info twice and the ini loader overwrites a repeated key instead of
        // keeping both, so the surrender line is the one that disappears. The second key takes the numbered name
        // the switch reader already looks for, and the extra byte comes out of the blank before its own equals.
        if (!replace_one(patched, "on_info = {+agro_ybiica_start} camper2",
            "on_info2 ={+agro_ybiica_start} camper2")) return false;
    }
    else if (repair == ConfigRepair::propysk_text)
    {
        if (!rename_propysk_strings(patched)) return false;
    }
    else if (repair == ConfigRepair::skill_banner)
    {
        if (patched.find("stalkerok_navuk_mex_lvl2") != patched.npos ||
            !replace_one(patched, mechanic_level_two, mechanic_level_two_fixed)) return false;
    }
    else if (repair == ConfigRepair::yantar_sensor)
    {
        if (!blank_to(patched, yantar_sensor_at, yantar_sensor_fix)) return false;
    }
    else if (repair == ConfigRepair::german_smg_names)
    {
        if (!replace_one(patched, mp40_name_at, mp40_name_fix) ||
            !replace_one(patched, mp41_name_at, mp41_name_fix)) return false;
    }
    else if (repair == ConfigRepair::b94_binding)
    {
        // The rifle asks to be bound to a script module that exists nowhere, so the lookup fails silently on
        // every spawn. The line is commented out at its exact length.
        if (!replace_one(patched, "script_binding  = bind_wpn.init", ";cript_binding  = bind_wpn.init"))
            return false;
    }
    else if (repair == ConfigRepair::trader_refusal)
    {
        // The Dark Valley seller of the only "Bogdan" artefact: turning down his 800 roubles ended the offer for good.
        if (!reopen_refused_offer(patched, "val_chr_torgash4_start", "val_chr_torgash4_pshel_nax")) return false;
    }
    else if (repair == ConfigRepair::vodka_task)
    {
        // The task still describes the vodka errand the quest once was; the quest itself completes on the
        // 20 000 roubles the sergeant asks for, and nothing in the mod ever counts bottles. Both objectives
        // are reworded to what the game actually checks, at their exact length: "save up 20 000 for the pass"
        // and "hand the money to the sergeant", in the mod's own CP1251.
        if (!replace_one(patched,
                "<text>\xEE\xF2\xFB\xF1\xEA\xE0\xF2\xFC 15 \xE1\xF3\xF2\xFB\xEB\xEE\xEA \xE2\xEE\xE4\xEA\xE8</text>",
                "<text>\xD1\xEA\xEE\xEF\xE8\xF2\xFC 20 000 \xED\xE0 \xEF\xF0\xEE\xEF\xF3\xF1\xEA</text>") ||
            !replace_one(patched,
                "<text>\xCF\xF0\xE8\xED\xE5\xF1\xF2\xE8 \xE2\xEE\xE4\xEA\xF3 \xF1\xF2\xE0\xF0\xF8\xE8\xED\xE5</text>",
                "<text>\xCE\xF2\xE4\xE0\xF2\xFC \xE4\xE5\xED\xFC\xE3\xE8 \xF1\xE5\xF0\xE6\xE0\xED\xF2\xF3.</text>"))
            return false;
    }
    else if (repair == ConfigRepair::car_text)
    {
        const auto begin = patched.find("<slom_jiga1_opisi");
        const auto end = patched.find("</slom_jiga1_opisi>", begin);
        if (begin == patched.npos || end == patched.npos) return false;
        auto contents = patched.substr(begin, end - begin);
        const auto old = contents.find("\xC4\xE2\xE5 \xF4\xE0\xF0\xFB");
        const auto indentation = contents.find("\n\t");
        if (old == contents.npos || indentation == contents.npos) return false;
        contents.replace(old, 8, "\xCE\xE4\xED\xE0 \xF4\xE0\xF0\xE0");
        contents.erase(indentation + 1, 1);
        patched.replace(begin, contents.size(), contents);
    }
    else if (repair == ConfigRepair::info_root)
    {
        constexpr std::string_view closing = "</game_information_portions>";
        if (patched.find("<game_information_portions>") == patched.npos) return false;
        if (patched.find(closing) != patched.npos) return false;
        const auto last = patched.find_last_not_of(" \t\r\n");
        if (last == patched.npos || patched.size() - last - 1 < closing.size() + 3) return false;
        patched.replace(last + 3, closing.size(), closing);
    }
    else
    {
        const auto owner = repair == ConfigRepair::skill_text ? block(patched, "string", "stanok_gg_mex_lvl0") :
            block(patched, "dialog", repair == ConfigRepair::mechanic_dialog ?
                "esc_l_d_mex_modern" : "esc_princ_persii_start");
        if (!owner) return false;
        auto contents = patched.substr(owner->begin, owner->end - owner->begin);
        if (repair == ConfigRepair::skill_text)
        {
            if (!replace_one(contents, "<<", std::string("\xAB ", 2)) ||
                !replace_one(contents, ">>", std::string(" \xBB", 2))) return false;
        }
        else if (repair == ConfigRepair::mechanic_dialog)
        {
            const auto branch = block(contents, "phrase", "1322");
            const auto first = block(contents, "phrase", "1331");
            const auto second = first ? block(contents, "phrase", "1331", first->end) : std::nullopt;
            if (!branch || !second || block(contents, "phrase", "1332")) return false;
            auto branch_text = contents.substr(branch->begin, branch->end - branch->begin);
            if (!replace_one(branch_text, "<next>1331</next>", "<next>1332</next>")) return false;
            contents.replace(branch->begin, branch_text.size(), branch_text);
            contents.replace(second->begin, 18, "<phrase id=\"1332\">");
        }
        else
        {
            for (const auto id : {"1", "2"})
            {
                const auto first = block(contents, "phrase", id);
                const auto second = first ? block(contents, "phrase", id, first->end) : std::nullopt;
                if (!first || !second || contents.substr(first->begin, first->end - first->begin) !=
                    contents.substr(second->begin, second->end - second->begin)) return false;
                for (auto at = second->begin; at < second->end; ++at)
                    if (contents[at] != '\r' && contents[at] != '\n') contents[at] = ' ';
            }
        }
        patched.replace(owner->begin, contents.size(), contents);
        if (repair == ConfigRepair::prince_dialog)
        {
            // Retain every implemented reward and state transition; remove only absent legacy action references.
            const auto ransom_present = block(patched, "dialog", "esc_dengi_rebe").has_value();
            if (!remove_absent_action(patched, "esc_sidor_artu_dolgy_ne_dal", "new_life.sidor_nagrada_1_6", 2) ||
                !remove_absent_action(patched, "esc_post_pianka", "new_life.give_albom", 1) ||
                !add_ransom_check(patched) || !add_tikhon_share(patched, ransom_present)) return false;
        }
    }
    if (patched.size() != text.size()) return false;
    text = std::move(patched);
    return true;
}

bool repair_config_buffer(std::span<std::byte> bytes)
{
    for (const auto& source : sources)
    {
        if (source.size != bytes.size()) continue;
        Sha256 hash{};
        if (!sha256_bytes(bytes, hash)) return false;
        constexpr char digits[] = "0123456789ABCDEF";
        std::string identity;
        for (const auto byte : hash)
        {
            const auto value = std::to_integer<unsigned>(byte);
            identity += digits[value >> 4];
            identity += digits[value & 15];
        }
        if (identity != source.hash) continue;
        std::string text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        if (!repair_config_text(text, source.repair)) return false;
        std::memcpy(bytes.data(), text.data(), text.size());
        return true;
    }
    return false;
}
}
