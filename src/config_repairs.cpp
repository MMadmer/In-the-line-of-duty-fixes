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
        "09B37C8A18C3183C5B2B87583BBA804162824B5AD65DF01660E78F4DE3F77810", ConfigRepair::vodka_task}
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
            if (!remove_absent_action(patched, "esc_sidor_artu_dolgy_ne_dal", "new_life.sidor_nagrada_1_6", 2) ||
                !remove_absent_action(patched, "esc_post_pianka", "new_life.give_albom", 1) ||
                !add_ransom_check(patched)) return false;
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
