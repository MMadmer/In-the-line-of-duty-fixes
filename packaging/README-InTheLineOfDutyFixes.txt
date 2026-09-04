In the Line of Duty Fixes

Installation:
Extract the complete archive into the Shadow of Chernobyl game root. Its bin and
gamedata directories merge with existing directories; no original file should be
replaced. Cancel extraction if it asks to replace an unknown existing file.

The complete runtime consists of the dinput8 loader, updater helper, uniquely named
Lua/UI files, and the .ild-fixes directory with the version, ownership and update
manifests. Do not install individual DLLs.

Repairs to mod scripts, configs and dialogs, and the whole Lua payload, work on any
build of the 1.0006 engine. The native tweaks below write to fixed addresses inside
the executable and are applied only on the exact binaries listed here; on any other
build each one skips itself and the rest of the fix pack still works.

Validated binaries (SHA-256):
  bin\XR_3DA.exe                   B22BC15B94A2A58C4E7046E46D46A3750D80C399BA8F37A2EF40CCF78EE3126D
  bin\xrCore.dll                   E6B6E0C150C4C511B299AA3C0E4E91D6B77A4801B23C9B6E55BF7A557ABEEEEB
  bin\xrGame.dll                   277B67FD6D21839A2F6C246EF57C8AD0C31079C0EAAAB179A8072D1B74A0284F
  bin\xrSound.dll                  741FE39CDB2081CADB7CAEE33C111C60BE7EE1248F01FFB6B8F550AF50BCEFEA
  bin\xrRender_R2.dll              2A91C9BB90A4CBF8A3E0F9265634A7F38ED19662B5B10089149FD1E7B2942F86
  gamedata\scripts\_g.script       2C5C2CCD95AE5B91F58C988D777C21444B832B746AFE3B565DF9A0E42F7AF2EE
  gamedata\scripts\bind_stalker.script   34732168AF8F7941A8BC87B7481A8A8686B447C27C25A914A11986D423B5C5B9
  gamedata\scripts\ui_main_menu.script   F18503040ED2FBBB84161857C0B55C84E8101CC911868978217A8E2C11577E08
Individual repairs additionally check the exact hash of the file they adapt.

Console editing:
Ctrl+A selects all. Shift+Left/Right/Home/End changes the selection. Ctrl+C copies
the selection (or the full line if nothing is selected). Ctrl+V pastes at the
caret. Ctrl+X cuts. Insert remains a paste shortcut. Selection brackets and the
caret are display-only and are never included in the executed command.

The helper uses Windows .NET Framework 4.x. It performs networking and applies
verified updates; the update dialog itself uses the game's existing UI classes.
It runs only while an update is offered and exits on its own otherwise. State it
keeps (status files, download cache, a failed-update note) lives under .ild-fixes.

Knife replacement uses the ordinary inventory equip action. The obsolete knife
warning is removed in memory; the main descriptions remain intact.
The incorrectly configured decorative table no longer runs a door controller.
Updater changes are paginated using the game's fonts and native buttons.

Additional repairs cover Unicode keyboard labels, sound metadata, stock graphics
presets, missing-bump fallbacks, model chunk boundaries, actor save data, dialog
graphs, detector callbacks, food use and complete recipe preconditions.
These fixes do not disable genuine engine diagnostics or overwrite original assets.

Added options:
Sound tab - "Radio volume", default 70%, in steps of 10. It scales every world
radio and music source and nothing else, and it takes effect as soon as it is
picked; 0% is complete silence, which is what a recording needs. The console
command "ild_update setting radio_volume <0..100>" takes any value in between,
and ild_radio_volume reports the one in force.
Video tab - "Screen mode": fullscreen, borderless window, or windowed. It is
applied when Apply is pressed, and the game rebuilds its render device itself.
A windowed mode is a real window: Alt+Tab and the Windows key work, and the
game no longer holds the display exclusively.
Both are stored in .ild-fixes\settings.txt, not in user.ltx, so removing the addon
leaves the game's own settings exactly as they were.

If something did not take effect:
Every launch writes .ild-fixes\runtime\loader-report.txt. It lists each file the
pack checks with its expected and actual SHA-256, and whether each repair applied.
Send that file when reporting a problem. If the file is absent, the loader never
ran: check that bin\dinput8.dll is present and that nothing else replaced it.

Removal:
Close the game and helper, remove the files listed in .ild-fixes\managed-files.txt,
then delete the .ild-fixes directory itself. Never delete the whole bin or gamedata
directory. The game/mod files and save format are not modified by the fix pack.
