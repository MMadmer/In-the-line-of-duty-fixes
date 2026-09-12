#include "config_repairs.h"
#include "item_descriptions.h"
#include "preset_compat.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>

int main(int argc, char** argv)
{
    using ild::ConfigRepair;
    std::string mechanic = "<dialog id=\"esc_l_d_mex_modern\"><phrase id=\"1331\"><action>first</action></phrase>"
        "<phrase id=\"1322\"><next>1331</next></phrase>"
        "<phrase id=\"1331\"><text>same_text_id</text><action>second</action></phrase></dialog>";
    const auto size = mechanic.size();
    if (!ild::repair_config_text(mechanic, ConfigRepair::mechanic_dialog) || mechanic.size() != size ||
        mechanic.find("<next>1332</next>") == mechanic.npos || mechanic.find("id=\"1332\"") == mechanic.npos ||
        mechanic.find("<action>second</action>") == mechanic.npos) return 1;
    auto unchanged = mechanic;
    if (ild::repair_config_text(mechanic, ConfigRepair::mechanic_dialog) || mechanic != unchanged) return 2;
    // The Cordon dialog file carries four repairs at once, so this fixture holds only what the first two
    // need; the reply list Sidorovich lost is checked on its own below, where the indentation it is paid
    // out of can be part of the fixture.
    std::string prince = "<dialog id=\"esc_princ_persii_start\"><phrase id=\"1\">A</phrase>"
        "<phrase id=\"2\">B</phrase><phrase id=\"1\">A</phrase><phrase id=\"2\">B</phrase></dialog>";
    if (!ild::repair_config_text(prince, ConfigRepair::prince_dialog) ||
        prince.find("id=\"1\"", prince.find("id=\"1\"") + 1) != prince.npos) return 3;
    // A reply list that lost one of its entries: Abram asks for a pistol, the mod writes both answers and
    // numbers them in their own text, and the phrase that asks names only the first. The reply goes back in
    // beside its sibling, paid for out of the block's own indentation, so the file keeps its length.
    std::string abram = "<dialog id=\"gar_stalk_baraxol3_k_baraxolke\">" "\r\n"
        "        <phrase id=\"6\">\r\n"
        "          <next>7</next>\r\n"
        "        </phrase>\r\n"
        "        <phrase id=\"8\"></phrase>\r\n"
        "      </dialog>";
    const auto abram_size = abram.size();
    if (!ild::repair_config_text(abram, ConfigRepair::abram_pistol)) return 41;
    if (abram.size() != abram_size) return 44;
    if (abram.find("<next>7</next><next>8</next>") == abram.npos) return 45;
    if (abram.find("<next>8</next>") != abram.rfind("<next>8</next>")) return 46;
    // A source that already offers the reply is not the file the repair was written for.
    if (ild::repair_config_text(abram, ConfigRepair::abram_pistol)) return 42;
    // A source without the dialog at all is left alone rather than failed, the way an absent action is.
    std::string other = "<dialog id=\"someone_else\"><phrase id=\"6\"></phrase></dialog>";
    const auto other_copy = other;
    if (!ild::repair_config_text(other, ConfigRepair::abram_pistol) || other != other_copy) return 43;
    std::string info = "<game_information_portions><info_portion id=\"x\"/>" + std::string(51, '\n');
    if (!ild::repair_config_text(info, ConfigRepair::info_root) || info.find("</game_information_portions>") == info.npos)
        return 4;
    std::string label = "<string id=\"stanok_gg_mex_lvl0\"><text>before <<skill:1>> after</text></string>";
    if (!ild::repair_config_text(label, ConfigRepair::skill_text) || label.find("<<") != label.npos ||
        label.find("skill:1") == label.npos) return 5;
    std::string camp = "[logic]\n active = {esc_delet_makar_i_tp} kamp2, kamp\n";
    if (!ild::repair_config_text(camp, ConfigRepair::camp_condition) ||
        camp.find("{+esc_delet_makar_i_tp}") == camp.npos) return 6;
    if (!ild::obsolete_preset_option(" rs_detail on") || ild::obsolete_preset_option("rs_detail_extra on") ||
        ild::obsolete_preset_option("r__tf_aniso 16")) return 7;
    // Only the material-5 phrase is retargeted; the material-7 phrase keeps its own reward.
    const std::string skat_phrases = "<text>b_bronevik_modern_5052</text><action>pochinka.b_mne_mod_skat57</action>\r\n"
        "<text>b_bronevik_modern_505</text>\r\n<action>pochinka.give_b_skat_komb</action>\r\n"
        "<action>pochinka.b_mne_mod_skat7</action>\r\n"
        "<text>b_bronevik_modern_705</text>\r\n<action>pochinka.b_mne_mod_skat7</action>\r\n";
    // Bronevik's detector offer, in the same file: refusing without money must not hide it for good.
    const std::string detector = "<dialog id=\"bar_bronevik_pochini_detektor\">\r\n"
        "<has_info>b_bronevik_strt</has_info>\r\n<dont_has_info>bar_bronevik_pochini_detektor</dont_has_info>\r\n"
        "<dont_has_info>bar_bronevik_ne_chini_detektor</dont_has_info>\r\n"
        "<phrase id=\"25\"><give_info>bar_bronevik_ne_chini_detektor</give_info>   \r\n"
        "<action>dialogs.break_dialog</action></phrase>\r\n"
        "<phrase id=\"11\"><give_info>bar_bronevik_pochini_detektor</give_info></phrase>\r\n</dialog>\r\n";
    // The wish-granter branch of the mechanic's drinking talk, in the same file: it must come back to the hub.
    const std::string wish = "<phrase id=\"17\">\r\n<text>b_nac_mex_bazar_17</text>\r\n"
        "<give_info>bar_nac_mexik_bazar2</give_info>\r\n<next>2</next>\r\n</phrase>\r\n"
        "<phrase id=\"18\">\r\n<next>2</next>\r\n</phrase>\r\n";
    // The added dialog is paid for out of indentation, so the source needs some to lend, as the real file has.
    std::string room;
    for (int line = 0; line < 40; ++line) room += "                    <phrase id=\"9\"></phrase>\r\n";
    std::string skat = skat_phrases + detector + wish + room;
    const auto skat_size = skat.size();
    if (!ild::repair_config_text(skat, ConfigRepair::skat_upgrade) || skat.size() != skat_size ||
        skat.find("b_mne_mod_skat5<") == std::string::npos ||
        skat.find("b_mne_mod_skat7") != skat.rfind("b_mne_mod_skat7") ||
        skat.find("b_mne_mod_skat57") == std::string::npos) return 12;
    if (skat.find("bar_bronevik_ne_chini_detektor") != std::string::npos ||
        skat.find("<dont_has_info>bar_bronevik_pochini_detektor</dont_has_info>") == std::string::npos ||
        skat.find("<give_info>bar_bronevik_pochini_detektor</give_info>") == std::string::npos ||
        skat.find("<action>dialogs.break_dialog</action>") == std::string::npos) return 25;
    if (skat.find("<give_info>bar_nac_mexik_bazar2</give_info>\r\n<next>4</next>") == std::string::npos ||
        skat.find("<phrase id=\"18\">\r\n<next>2</next>") == std::string::npos) return 33;
    // Bronevik's missing hand-over: gated on holding both halves, so it closes itself once they are spent.
    if (skat.find("<dialog id=\"ild_bronevik_detektor_gotov\">") == std::string::npos ||
        skat.find("<precondition>ild_script_repairs.detector_parts_ready</precondition>") == std::string::npos ||
        skat.find("<action>ild_script_repairs.repair_detector</action>") == std::string::npos ||
        skat.find("<has_info>bar_bronevik_pochini_detektor</has_info>") == std::string::npos) return 34;
    // A source that already carries it is not the file the repair was written for.
    std::string already_added = skat;
    if (ild::repair_config_text(already_added, ConfigRepair::skat_upgrade)) return 35;
    // Bronevik's own topic list, in the character file, is what puts the conversation in front of the player.
    std::string profile = "\t\t<actor_dialog>b_bronevik_modern</actor_dialog>\r\n"
        "\t\t<actor_dialog>bar_bronevik_pochini_detektor</actor_dialog>\r\n" + room;
    const auto profile_size = profile.size();
    if (!ild::repair_config_text(profile, ConfigRepair::bronevik_profile) || profile.size() != profile_size ||
        profile.find("<actor_dialog>ild_bronevik_detektor_gotov</actor_dialog>") == std::string::npos ||
        profile.find("<actor_dialog>bar_bronevik_pochini_detektor</actor_dialog>") == std::string::npos) return 36;
    if (ild::repair_config_text(profile, ConfigRepair::bronevik_profile)) return 37;
    std::string no_bronevik = "\t\t<actor_dialog>someone_else</actor_dialog>\r\n" + room;
    if (ild::repair_config_text(no_bronevik, ConfigRepair::bronevik_profile)) return 38;
    // The PDA entry the job never had: only the skeleton, its two steps are built where the targets are known.
    std::string tasks = room;
    const auto tasks_size = tasks.size();
    if (!ild::repair_config_text(tasks, ConfigRepair::detector_task) || tasks.size() != tasks_size ||
        tasks.find("<game_task id=\"ild_detector_task\">") == std::string::npos ||
        // The title is the text itself, not an id: the engine never opens a string file of the pack's own.
        tasks.find("<title>\xD0\xE5\xEC\xEE\xED\xF2\x20\xED\xE0\xF3\xF7\xED\xEE\xE3\xEE\x20"
            "\xE4\xE5\xF2\xE5\xEA\xF2\xEE\xF0\xE0</title>") == std::string::npos) return 39;
    if (ild::repair_config_text(tasks, ConfigRepair::detector_task)) return 40;
    // The two lines that dialog names live in the Bar's own string table, the file the engine really opens.
    // That same file declares the crate courier's reply under the previous phrase's id, so one line printed
    // the wrong text and the next printed its identifier raw; the duplicate is split in the same pass.
    std::string lines = "<string_table>\r\n"
        "        <string id=\"bar_iahik_nac_2_1_0\"><text>A</text></string>\r\n"
        "        <string id=\"bar_iahik_nac_2_1_0\"><text>B</text></string>\r\n"
        + room;
    const auto lines_size = lines.size();
    if (!ild::repair_config_text(lines, ConfigRepair::detector_text) || lines.size() != lines_size) return 47;
    if (lines.find("<string id=\"ild_bronevik_detektor_gotov_0\">") == std::string::npos ||
        lines.find("<string id=\"ild_bronevik_detektor_gotov_1\">") == std::string::npos ||
        lines.find("EVA-1400") == std::string::npos) return 48;
    // The courier's reply now carries the id the dialog points at, and only the second block moved.
    if (lines.find("bar_iahik_nac_2_1_1") == std::string::npos ||
        lines.find("bar_iahik_nac_2_1_0") == std::string::npos ||
        lines.find("bar_iahik_nac_2_1_0") != lines.rfind("bar_iahik_nac_2_1_0")) return 49;
    if (ild::repair_config_text(lines, ConfigRepair::detector_text)) return 50;
    // Without the detector dialog this is not the file the repair was written for.
    std::string skat_only = skat_phrases;
    unchanged = skat_only;
    if (ild::repair_config_text(skat_only, ConfigRepair::skat_upgrade) || skat_only != unchanged) return 26;
    std::string absent = "<text>b_bronevik_modern_505</text>\r\n<action>pochinka.other</action>\r\n";
    if (ild::repair_config_text(absent, ConfigRepair::skat_upgrade)) return 13;
    std::string ambiguous = "<text>b_bronevik_modern_505</text><text>b_bronevik_modern_505</text>";
    if (ild::repair_config_text(ambiguous, ConfigRepair::skat_upgrade)) return 14;
    // The two absent module-scope sounds become nil; both use sites already test the value.
    std::string mutants = "local insect_sound = sound_object([[anomaly\\flies]])\r\nlocal phantom_sound = sound_object([[monsters\\phantom\\phantom_snork_death]])\r\n"
        "local zombie_sound2 = sound_object([[weapons\\f1_explode_]])\r\n";
    const auto mutants_size = mutants.size();
    if (!ild::repair_config_text(mutants, ConfigRepair::mutant_sounds) || mutants.size() != mutants_size ||
        mutants.find("insect_sound = nil ") == std::string::npos ||
        mutants.find("phantom_sound = nil ") == std::string::npos ||
        mutants.find("f1_explode_") == std::string::npos) return 15;
    std::string once = "local insect_sound = sound_object([[anomaly\\flies]])\r\n";
    if (ild::repair_config_text(once, ConfigRepair::mutant_sounds)) return 16;
    // The ransom dialog gains its money gate and charge; the block's own indentation pays for every byte.
    const std::string prince_part = "<dialog id=\"esc_princ_persii_start\"><phrase id=\"1\">A</phrase>"
        "<phrase id=\"2\">B</phrase><phrase id=\"1\">A</phrase><phrase id=\"2\">B</phrase></dialog>\r\n";
    const std::string indent(60, ' ');
    const std::string reba = "<dialog id=\"esc_dengi_rebe\">\r\n<has_info>esc_tixon_atp_final</has_info>\r\n"
        "<dont_has_info>esc_dengi_rebe</dont_has_info>\r\n" + indent + "<phrase_list>\r\n" + indent +
        "<phrase id=\"4\">\r\n" + indent + "<text>esc_dengi_rebe_4</text> \r\n"
        "<action>new_life.sidor_mne_keis_s_artami</action>\r\n\t<give_info>esc_dengi_rebe</give_info>   \r\n" +
        indent + "</phrase>\r\n" + indent + "</phrase_list>\r\n </dialog>\r\n";
    // Tikhon's closing dialog, whose "here is your share" line pays nothing in the mod.
    const std::string tikhon = "<dialog id=\"esc_tixon_atp_final\">\r\n"
        "<has_info>esc_atp_ydalai_ysex_k_xyiam_end</has_info>\r\n" +
        indent + "<phrase_list>\r\n" + indent + "<phrase id=\"1\">\r\n" + indent +
        "<text>esc_tixon_atp_final_1</text>    \r\n" + indent + "<next>2</next>\r\n" + indent + "</phrase>\r\n" +
        indent + "</phrase_list>\r\n </dialog>\r\n";
    std::string ransom = prince_part + tikhon + reba;
    const auto ransom_size = ransom.size();
    if (!ild::repair_config_text(ransom, ConfigRepair::prince_dialog) || ransom.size() != ransom_size ||
        ransom.find("<dont_has_info>esc_dengi_rebe</dont_has_info>"
            "<precondition>new_life.don_reba_denga_za_artu_esti</precondition>") == std::string::npos ||
        ransom.find("<action>new_life.ia_otdaq_rebe_dengy_250000</action>"
            "<action>new_life.sidor_mne_keis_s_artami</action>") == std::string::npos ||
        ransom.find(" <text>esc_dengi_rebe_4</text> \r\n") == std::string::npos ||
        ransom.find("\t<give_info>esc_dengi_rebe</give_info>   \r\n") == std::string::npos ||
        ransom.find("\r\n <phrase_list>") == std::string::npos) return 17;
    if (ransom.find("<text>esc_tixon_atp_final_1</text><action>ild_script_repairs.tikhon_share</action>    \r\n") ==
        std::string::npos || ransom.find("<next>2</next>") == std::string::npos) return 27;
    unchanged = ransom;
    if (ild::repair_config_text(ransom, ConfigRepair::prince_dialog) || ransom != unchanged) return 18;
    // The charge never goes in without the share that pays for it.
    std::string unpaid = prince_part + reba;
    unchanged = unpaid;
    if (ild::repair_config_text(unpaid, ConfigRepair::prince_dialog) || unpaid != unchanged) return 28;
    // The Dark Valley seller: the refusal gate and its writer go, the purchase still closes the offer.
    std::string trader = "<dialog id=\"val_chr_torgash4_start\">\r\n"
        "<dont_has_info>val_chr_torgash4_pshel_nax</dont_has_info>\r\n"
        "<dont_has_info>val_chr_torgash4_kypil_art</dont_has_info>\r\n"
        "<phrase id=\"18\"><give_info>val_chr_torgash4_pshel_nax</give_info>      \r\n"
        " <action>dialogs.break_dialog</action></phrase>\r\n"
        "<phrase id=\"17\"><give_info>val_chr_torgash4_kypil_art</give_info></phrase>\r\n</dialog>\r\n";
    const auto trader_size = trader.size();
    if (!ild::repair_config_text(trader, ConfigRepair::trader_refusal) || trader.size() != trader_size ||
        trader.find("pshel_nax") != std::string::npos ||
        trader.find("<dont_has_info>val_chr_torgash4_kypil_art</dont_has_info>") == std::string::npos ||
        trader.find("<give_info>val_chr_torgash4_kypil_art</give_info>") == std::string::npos) return 29;
    unchanged = trader;
    if (ild::repair_config_text(trader, ConfigRepair::trader_refusal) || trader != unchanged) return 30;
    // The shovel's description stops calling it short-lived; every other item and text is returned untouched.
    const std::string shovel = "\xCD\xE5\xEF\xEB\xEE\xF5\xE0\xFF \xF8\xF2\xFB\xEA\xEE\xE2\xE0\xFF "
        "\xEB\xEE\xEF\xE0\xF2\xE0, \xF5\xEE\xF2\xFC \xE8 \xED\xE5 \xE4\xEE\xEB\xE3\xEE\xE2\xE5\xF7\xED\xE0, "
        "\xED\xEE \xE2\xE5\xF1\xFC\xEC\xE0 \xEF\xEE\xEB\xE5\xE7\xED\xE0.";
    const auto repaired = ild::repaired_description("item_lopata", shovel);
    if (!repaired || repaired->find("\xE4\xEE\xEB\xE3\xEE\xE2\xE5\xF7\xED\xE0") != std::string::npos ||
        repaired->find("\xEA\xF0\xE5\xEF\xEA\xE0\xFF \xE8 \xED\xE0\xE4\xB8\xE6\xED\xE0\xFF, \xE4\xE0 \xE8 "
            "\xE2\xE5\xF1\xFC\xEC\xE0") == std::string::npos) return 31;
    if (ild::repaired_description("esc_lopata_kopatelia_veshi", shovel) ||
        ild::repaired_description("item_lopata", "\xDD\xF2\xEE \xEB\xEE\xEF\xE0\xF2\xE0.")) return 32;
    std::string cramped = prince_part + "<dialog id=\"esc_dengi_rebe\">\r\n<dont_has_info>esc_dengi_rebe</dont_has_info>\r\n"
        "<action>new_life.sidor_mne_keis_s_artami</action>\r\n</dialog>\r\n";
    unchanged = cramped;
    if (ild::repair_config_text(cramped, ConfigRepair::prince_dialog) || cramped != unchanged) return 19;
    // Both objectives of the vodka task are reworded in place, in CP1251 and at their exact length.
    const std::string old_first = "<text>\xEE\xF2\xFB\xF1\xEA\xE0\xF2\xFC 15 \xE1\xF3\xF2\xFB\xEB\xEE\xEA \xE2\xEE\xE4\xEA\xE8</text>";
    const std::string old_second = "<text>\xCF\xF0\xE8\xED\xE5\xF1\xF2\xE8 \xE2\xEE\xE4\xEA\xF3 \xF1\xF2\xE0\xF0\xF8\xE8\xED\xE5</text>";
    std::string vodka = "<string id=\"esc_kom_vodka_0\">\r\n        " + old_first + "\r\n    </string>\r\n"
        "    <string id=\"esc_kom_vodka_1\">\r\n        " + old_second + "\r\n    </string>\r\n";
    const auto vodka_size = vodka.size();
    if (!ild::repair_config_text(vodka, ConfigRepair::vodka_task) || vodka.size() != vodka_size ||
        vodka.find("<text>\xD1\xEA\xEE\xEF\xE8\xF2\xFC 20 000 \xED\xE0 \xEF\xF0\xEE\xEF\xF3\xF1\xEA</text>") == std::string::npos ||
        vodka.find("<text>\xCE\xF2\xE4\xE0\xF2\xFC \xE4\xE5\xED\xFC\xE3\xE8 \xF1\xE5\xF0\xE6\xE0\xED\xF2\xF3.</text>") == std::string::npos ||
        vodka.find("15 ") != std::string::npos) return 20;
    std::string twice = old_first + old_first + old_second;
    unchanged = twice;
    if (ild::repair_config_text(twice, ConfigRepair::vodka_task) || twice != unchanged) return 21;

    // The contact counter moves onto the dial the mod paints, at the file's exact length.
    std::string counter =
        "<window>\r\n"
        "\t<static_pda_online x=\"104\" y=\"167\" width=\"27\" height=\"28\" la_text=\"1\" stretch=\"1\">\r\n"
        "\t\t<texture>ui_hud_map_counter</texture>\r\n"
        "\t\t<text y=\"4\" align=\"c\" font=\"graffiti19\"/>\r\n"
        "\t</static_pda_online>\r\n</window>\r\n";
    const auto counter_size = counter.size();
    if (!ild::repair_config_text(counter, ConfigRepair::counter_wide) || counter.size() != counter_size ||
        counter.find("<static_pda_online x=\"105\" y=\"153\" width=\"27\"") == std::string::npos ||
        counter.find("y=\"167\"") != std::string::npos) return 22;
    // A source that is not the one the numbers were measured against is left exactly as it is.
    std::string foreign = counter;
    if (ild::repair_config_text(foreign, ConfigRepair::counter_wide) || foreign != counter) return 23;
    std::string narrow =
        "<window>\r\n"
        "\t<static_pda_online x=\"138\" y=\"167\" width=\"35\" height=\"28\" la_text=\"1\">\r\n"
        "\t\t<texture>ui_hud_map_counter</texture>\r\n"
        "\t\t<text y=\"6\" align=\"c\" font=\"graffiti19\"/>\r\n"
        "\t</static_pda_online>\r\n</window>\r\n";
    const auto narrow_size = narrow.size();
    if (!ild::repair_config_text(narrow, ConfigRepair::counter_normal) || narrow.size() != narrow_size ||
        narrow.find("<static_pda_online x=\"135\" y=\"148\" width=\"35\"") == std::string::npos) return 24;

    if (argc == 3)
    {
        const std::filesystem::path root(argv[1]), output(argv[2]);
        for (const auto& source : ild::config_repair_sources())
        {
            std::ifstream input(root / source.relative, std::ios::binary);
            if (!input)
            {
                // Some sources live inside the game archives; those are only reachable at run time.
                std::wcout << source.relative << L": archived, checked at run time\n";
                continue;
            }
            std::string text(std::istreambuf_iterator<char>(input), {});
            if (text.size() != source.size) return 8;
            const auto original = text;
            if (!ild::repair_config_buffer(std::as_writable_bytes(std::span(text)))) return 9;
            if (text == original || text.size() != original.size()) return 10;
            const auto target = output / source.relative;
            std::filesystem::create_directories(target.parent_path());
            std::ofstream stream(target, std::ios::binary);
            stream.write(text.data(), static_cast<std::streamsize>(text.size()));
            if (!stream) return 11;
            std::wcout << source.relative << L": repaired, size preserved\n";
        }
    }
    return 0;
}
