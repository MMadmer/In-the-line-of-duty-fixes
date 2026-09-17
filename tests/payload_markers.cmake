# The shipped scripts are read by players and by the engine; staging notes with developer paths, "append this
# block" instructions or references to modules that do not exist have no place in them. Every marker below was
# found in a shipped file once.
file(GLOB SCRIPTS "${SOURCE_ROOT}/payload/gamedata/scripts/*.script")
set(MARKERS "APPEND VERBATIM" "WIRING (" "APPLY:" "Ships as " "In-the-line-of-duty-fixes" "D:\\\\Games")
set(MODULES ild_fix_ui ild_gameplay ild_script_repairs ild_recipe_repairs ild_mod_repairs ild_quest_repairs ild_qa)
foreach(SCRIPT ${SCRIPTS})
    file(READ "${SCRIPT}" TEXT)
    foreach(MARKER ${MARKERS})
        string(FIND "${TEXT}" "${MARKER}" POSITION)
        if(NOT POSITION EQUAL -1)
            message(FATAL_ERROR "${SCRIPT} carries a staging note: ${MARKER}")
        endif()
    endforeach()
    # Every ild_* module a script names must ship, or the engine resolves it to nil at the first touch.
    string(REGEX MATCHALL "ild_[a-z_]+\\.(script|install)" NAMED "${TEXT}")
    foreach(NAME ${NAMED})
        string(REGEX REPLACE "\\.(script|install)$" "" MODULE "${NAME}")
        list(FIND MODULES "${MODULE}" KNOWN)
        if(KNOWN EQUAL -1)
            message(FATAL_ERROR "${SCRIPT} names a module that does not ship: ${MODULE}")
        endif()
    endforeach()
endforeach()
