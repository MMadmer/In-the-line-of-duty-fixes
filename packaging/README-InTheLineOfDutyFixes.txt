In the Line of Duty Fixes

Installation:
Extract the complete archive into the Shadow of Chernobyl game root. Its bin and
gamedata directories merge with existing directories; no original file should be
replaced. Cancel extraction if it asks to replace an unknown existing file.

The complete runtime consists of the dinput8 loader, updater helper, uniquely named
Lua/UI files, and the .ild-fixes directory with the version, ownership and update
manifests. Do not install individual DLLs.

Repairs to mod scripts, configs and dialogs, the whole Lua payload, the in-game
updater and the added options work on any build of the 1.0006 engine, with or
without the mod's own binaries: the updater checks the engine's console command
layout against the executable's own exports, not its hash. The native tweaks below
write to fixed addresses inside the binaries and apply only where the code at
their patch site is the validated one; on any other build each one skips itself
without a message, the loader report says so, and the rest of the pack still works.
An installation of 1.0.3 or older without the mod's binaries never saw the update
window, so it has to install this version by hand once.

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

Stalled quest NPCs are detected and unstuck. Scripted NPC logic in this engine
waits on engine callbacks that have no timeout, so a route the engine cannot
walk, or an animation it cannot reach, leaves an NPC standing or sitting for
good - the fault behind an NPC that stops in a doorway, or reaches a stash and
never reports back. The fix pack watches for those waits and re-runs what the
game itself would have run; a wait that is meant to end on a signal or an info
portion is never forced, so no scripted step is skipped. Each intervention is
recorded in .ild-fixes\runtime\npc-watchdog.txt.

Quest repairs, each in memory only: the Don Reba ransom checks and takes the
250 000 its dialog, task and check functions describe, and Tikhon's "here is your
share" line at the ATP pays exactly that sum, once; the "Save up a quarter million"
objective completes while the sum is held; the "Alcohol Wars" task text names the
20 000 the sergeant actually asks for; the stash Sidorovich sells on Cordon, and
any other sealed box the player is pointed at, becomes searchable; the Burglar
skill's first level is both of its journals, so Tikhon's storeroom and the Bar
autopark safe open as their tips promise; Bronevik's detector inspection and the
Dark Valley trader's "Bogdan" artefact stay on offer after a visit without money;
a scripted mutant that dies out of sight still delivers the info portions of its
death section, unless a mod script removed it; and a killer-less mutant death no
longer aborts its scheme.
The Wild Territory rally, in memory as well: the opponents' trucks keep driving
while nobody looks at them (where the game DLL matches the validated build every
car stays in the engine's per-frame update, as an armed one always did; elsewhere
a truck the physics shows moving is not judged stuck), and the track behind the
level's invisible walls has a way out. After the organizer's closing words the
player is moved to the ordinary side of the door; a finish reached at the wheel
still returns the player to the organizer; and walking back onto the spot the
door brought the player to and standing there for three seconds leads out at any
time.

Shovels, a deliberate change requested for the pack: digging a grave stash no
longer takes the shovel, so one shovel lasts for every grave. Stamina, sounds,
loot and each grave closing after one dig are unchanged.

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
pack checks with its expected and actual SHA-256, whether each repair installed,
the stored options, and whether Windows redirects what the game writes into its
folder (UAC virtualization in a protected folder such as Program Files); while the
game runs it adds whether the console spam fix and the script bindings took effect.
Send that file when reporting a problem. A windowed screen mode also leaves
.ild-fixes\runtime\window-state.txt: a few lines naming the window the pack found
and the style it applied. If the loader report is absent, the loader never ran:
check that bin\dinput8.dll is present and that nothing, an antivirus included,
removed or replaced it.

Removal:
Close the game and helper, remove the files listed in .ild-fixes\managed-files.txt,
then delete the .ild-fixes directory itself. Never delete the whole bin or gamedata
directory. The game/mod files and save format are not modified by the fix pack.
