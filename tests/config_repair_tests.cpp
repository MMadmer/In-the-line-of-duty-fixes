#include "config_repairs.h"
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
    std::string prince = "<dialog id=\"esc_princ_persii_start\"><phrase id=\"1\">A</phrase>"
        "<phrase id=\"2\">B</phrase><phrase id=\"1\">A</phrase><phrase id=\"2\">B</phrase></dialog>";
    if (!ild::repair_config_text(prince, ConfigRepair::prince_dialog) ||
        prince.find("id=\"1\"", prince.find("id=\"1\"") + 1) != prince.npos) return 3;
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
    std::string skat = "<text>b_bronevik_modern_5052</text><action>pochinka.b_mne_mod_skat57</action>\r\n"
        "<text>b_bronevik_modern_505</text>\r\n<action>pochinka.give_b_skat_komb</action>\r\n"
        "<action>pochinka.b_mne_mod_skat7</action>\r\n"
        "<text>b_bronevik_modern_705</text>\r\n<action>pochinka.b_mne_mod_skat7</action>\r\n";
    const auto skat_size = skat.size();
    if (!ild::repair_config_text(skat, ConfigRepair::skat_upgrade) || skat.size() != skat_size ||
        skat.find("b_mne_mod_skat5<") == std::string::npos ||
        skat.find("b_mne_mod_skat7") != skat.rfind("b_mne_mod_skat7") ||
        skat.find("b_mne_mod_skat57") == std::string::npos) return 12;
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
    std::string ransom = prince_part + "<dialog id=\"esc_dengi_rebe\">\r\n<has_info>esc_tixon_atp_final</has_info>\r\n"
        "<dont_has_info>esc_dengi_rebe</dont_has_info>\r\n" + indent + "<phrase_list>\r\n" + indent +
        "<phrase id=\"4\">\r\n" + indent + "<text>esc_dengi_rebe_4</text> \r\n"
        "<action>new_life.sidor_mne_keis_s_artami</action>\r\n\t<give_info>esc_dengi_rebe</give_info>   \r\n" +
        indent + "</phrase>\r\n" + indent + "</phrase_list>\r\n </dialog>\r\n";
    const auto ransom_size = ransom.size();
    if (!ild::repair_config_text(ransom, ConfigRepair::prince_dialog) || ransom.size() != ransom_size ||
        ransom.find("<dont_has_info>esc_dengi_rebe</dont_has_info>"
            "<precondition>new_life.don_reba_denga_za_artu_esti</precondition>") == std::string::npos ||
        ransom.find("<action>new_life.ia_otdaq_rebe_dengy_250000</action>"
            "<action>new_life.sidor_mne_keis_s_artami</action>") == std::string::npos ||
        ransom.find(" <text>esc_dengi_rebe_4</text> \r\n") == std::string::npos ||
        ransom.find("\t<give_info>esc_dengi_rebe</give_info>   \r\n") == std::string::npos ||
        ransom.find("\r\n <phrase_list>") == std::string::npos) return 17;
    unchanged = ransom;
    if (ild::repair_config_text(ransom, ConfigRepair::prince_dialog) || ransom != unchanged) return 18;
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

    if (argc == 3)
    {
        const std::filesystem::path root(argv[1]), output(argv[2]);
        for (const auto& source : ild::config_repair_sources())
        {
            std::ifstream input(root / source.relative, std::ios::binary);
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
