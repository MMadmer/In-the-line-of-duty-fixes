# Console compatibility

The supported `XR_3DA.exe` SHA-256 is
`B22BC15B94A2A58C4E7046E46D46A3750D80C399BA8F37A2EF40CCF78EE3126D`.
Do not reuse its offsets against another executable.

## Confirmed causes

The mod's `_g.script` calls
`get_console():execute(string.gsub(fmt, " ", "_"))` from `printf` before its
normal formatted log call. This turns debug format strings into invalid engine
commands. The fix removes only that statement from a private pagefile-backed
mapping, leaving the file on disk unchanged and keeping genuine logging intact.

The exact SoC executable has an older console than the later X-Ray line editor:

- `CConsole::Execute`, RVA `0xB9A30`, copies programmatic commands into the same
  1024-byte input buffer at object offset `56`, then calls `ExecuteCommand`.
- `ExecuteCommand`, RVA `0xB9630`, clears that input buffer even when the command
  came from Lua. This explains why active script calls erase unfinished input.
- `OnPressKey`, RVA `0xB8740`, appends characters and provides clipboard paste
  only on Insert. It has no selection or Ctrl+C/Ctrl+V handling.
- `OnRender`, RVA `0xB83A0`, receives the `pureRender` subobject at offset `4`.
  The complete console object's cursor blink flag is at offset `1080`.

The native hooks preserve the original command dispatcher and its diagnostics,
restore the edited input after programmatic execution, and implement a bounded
caret/selection model. Selection brackets are presentation-only. Native scan-code
translation, history, completion, and log scrolling still use the engine paths.

## Verification status

- Unit tests cover script patching and 14 text-editor cases.
- A repository-local run against the supported executable confirmed input
  preservation, Ctrl+A selection, replacement of a selection, and insertion at
  the caret through the native scan-code path.
- The copied `all.sav` reached level configuration completion. The 20-second
  run still encountered the pre-existing `ph_door.script:31` nil `joint` error
  for `esc_tixona_xyinia4`, also present in the original unmodified game log.
- The separate Windows clipboard API roundtrip succeeded. Its original test
  cleanup failed; the prior user clipboard item was restored from Windows
  history. That test has been replaced by a private-window-station test which
  refuses to touch the user clipboard if isolation is unavailable.
- A true hidden Desktop failed D3D9 initialization on this machine. The loaded
  save test used a hidden window on the normal graphics Desktop instead. This
  is not proof of fullscreen rendering or hidden-Desktop compatibility.

The updater applier passed 17 checks on synthetic installations: full and patch
equivalence, restart, original-file/save preservation, patch rejection, full
fallback, invalid digest rejection, destination collisions, and rollback.
The installed 0.9.0 updater also applied the actual 0.9.1 release archive on an
isolated installation: all seven target hashes matched, protected files and the
copied save were unchanged, and the restart stub completed.
The main game contains the complete 0.9.0 payload, intentionally retained as the
update source for 0.9.1. The in-game update dialog still requires visual/runtime QA;
the applier tests do not establish that the dialog renders or receives input.
