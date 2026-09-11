# Console compatibility

Later gameplay and visual verification supersede the earlier limitations below;
see [Gameplay and UI](GAMEPLAY_AND_UI.md).

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

## Update UI startup correction (2026-09-02)

The supported Lua environment does not expose `io` or `os`. The original addon
failed at `ild_fix_ui.script:8` during menu creation. A hash-gated native console
command now exchanges bounded status fields and whitelisted actions with the
helper; it never enables unrestricted Lua filesystem access. Status must match
the current game PID. Polling uses monotonic native time because game time stops
while the menu is paused.

Opening another holder dialog from the main menu's Update invalidated the active
dialog vector. The minidump and `xrGame.dll` function at `0x103B95B0` confirmed the
iteration path. The addon now attaches an owned child, hides the normal menu while
it is open, forwards keyboard input, and restores the menu on dismissal.

- The ordinary UI/download check reached `state=ready` through the real Lua action
  handler, then exited normally: 5-second soak, exit 0, no forced termination.
- The renderer's shutdown resource counters returned to zero.
- The latest-save check reached game configuration completion, then hit the
  existing `ph_door.script:31` error. It is not a clean gameplay pass.
- Hidden-Desktop D3D9 initialization remains unavailable. Tests used an offscreen,
  non-activating window at 2560x1440. Backbuffer captures were black, so visual
  appearance and foreground/fullscreen interaction remain unverified. The failed
  capture probe was removed from the build.
- Script patch, editor, and UI compatibility-contract tests pass. The updater's
  synthetic full/patch/rollback suite passes 37 checks; published 0.9.1 discovery
  and download were also exercised by the 0.9.0 client.

The published release is 1.0.0 while the installed game deliberately stays on
0.9.1, so the update path can be exercised from a real installation. Because the
major version differs, a 0.9.1 client offers 1.0.0 as a separate installation with
a link rather than downloading it, which is the documented behaviour for a major
step. `Deploy.cmd` requests UAC and deploys the complete current packaged runtime
through `tools/deploy/Deploy-FixPack.ps1`, verifying all eleven hashes and the save
tree and retaining a recoverable backup. Packaging must be refreshed whenever
runtime sources change.

## Console bridge without the executable digest (2026-09-11)

Until 1.0.3 the `ild_update` bridge, and with it the update window and the added
options, was installed only when `XR_3DA.exe` and `ui_main_menu.script` matched
their digests. An installation without the mod's patched binaries never saw an
update. The bridge now validates the one thing it depends on, the command layout,
against the executable's own exports:

- `??_7IConsole_Command@@6B@` holds the exported `Status`, `Info` and `Save` in
  slots 2-4, slots 0-1 point at code and slot 5 does not.
- `??0IConsole_Command@@QAE@PBD@Z`, run on a zeroed 32-byte buffer, writes the
  vtable at offset 0, the name at 4, the enabled flag (1) at 8 and two flags at
  9-10, and nothing else; `?Name@IConsole_Command@@QAEPBDXZ` returns the name.
  In the validated build the constructor is `mov [eax+4],name; mov [eax],vtable;
  mov [eax+8],1; mov [eax+9],1; mov byte [eax+10],0`.
- `CConsole::GetString` returns NULL for an unknown command, which Lua receives as
  nil; the payload uses that to skip every verb when the bridge is absent, since
  executing an unknown command logs an error line per call.

The menu is bound by path and by its unique `self:InitControls()` anchor, through
the `_read` import, which is installed on every build. Staging QA with a changed
`XR_3DA.exe` digest reported the identity as `MISMATCH` and still installed the
bridge and bound the menu. The QA DLL renders an inactive window; a QA probe must
time itself with `ild_update read clock`, because game time is paused while the
window is inactive.
