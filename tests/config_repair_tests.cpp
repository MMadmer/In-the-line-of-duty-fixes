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
