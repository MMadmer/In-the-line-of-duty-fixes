In the Line of Duty Fixes — development candidate

This candidate is not a verified release. The in-game updater UI and full update
flow still require runtime QA. Do not treat a successful build as release approval.

Installation of a verified release:
Extract the complete archive into the Shadow of Chernobyl game root. Its bin and
gamedata directories merge with existing directories; no original file should be
replaced. Cancel extraction if it asks to replace an unknown existing file.

The complete runtime consists of the dinput8 loader, updater helper, uniquely named
Lua/UI files, version and ownership manifests. Do not install individual DLLs.

Console editing:
Ctrl+A selects all. Shift+Left/Right/Home/End changes the selection. Ctrl+C copies
the selection (or the full line if nothing is selected). Ctrl+V pastes at the
caret. Ctrl+X cuts. Insert remains a paste shortcut. Selection brackets and the
caret are display-only and are never included in the executed command.

The helper uses Windows .NET Framework 4.x. It performs networking and applies
verified updates; the update dialog itself uses the game's existing UI classes.

Removal:
Close the game and helper, then remove only this release's files listed in
.ild-fixes/managed-files.txt. Never delete the whole bin or gamedata directory.
The game/mod files and save format are not modified by the fix pack.
