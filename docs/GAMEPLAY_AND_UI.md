# Gameplay and update UI fixes — 2026-09-02

## Supported identities and behavior

- `xrGame.dll` SHA-256:
  `277B67FD6D21839A2F6C246EF57C8AD0C31079C0EAAAB179A8072D1B74A0284F`.
- `bind_stalker.script` SHA-256:
  `34732168AF8F7941A8BC87B7481A8A8686B447C27C25A914A11986D423B5C5B9`.
- The actor spawn entry is redirected in its private read/mapping buffer. This
  installs gameplay fixes even when startup loads a save without constructing
  the main menu. Script size and original on-disk bytes remain unchanged.

The spawn entry `esc_tixona_xyinia4` names the table `physics\stol\stol_4`, but
its custom data refers to `scripts\door_logic.ltx`. It cannot have the expected
door hinge. Only that exact object/config combination receives a non-interactive
decoration action. Its physics object, binder, stored scheme, and save layout
remain intact; real doors retain their original validation and behavior.

`CUIInventoryWnd::ToSlot` at RVA `0x3BBF80` asks `GetSlotList` for knife slot zero.
The supported executable only returns lists for slots 1, 2, and 6. A null list
is then dereferenced, explaining the documented knife-replacement crash.
The fix uses the original inventory Ruck/Slot calls and original network events,
then requests a delayed bag rebuild. Other equipment retains the original path.
The module hash and whole-instruction hook signature are checked first.

The `CInifile::r_string` import is intercepted only for `description` in the five
affected knife sections. It removes the known warning suffix after verifying its
exact prefix. Everything before that suffix is returned unchanged in stable
storage. No weapon config, string table, or archive is overwritten.

## Update window

The title explicitly names **In the Line of Duty Fixes**, not the base mod.
The inventory texture sheet was replaced by an existing menu-dialog texture.
Buttons use their native dimensions; long text is measured with the game's fonts
and paginated. This avoids the broken scroll-thumb geometry in this installation.
Empty intermediate status snapshots no longer clear the window. A verified
cached download now reports its actual completed size instead of zero bytes.

## QA evidence

- Copied latest `all.sav`, renderer R2, 2560x1440, non-activating offscreen window.
- The actor supplied a readiness marker; its normal update ran for five seconds
  and requested a normal engine quit. The earlier door fatal did not recur.
- Six knife checks include the first automatic equip and five replacements via
  the actual patched inventory UI entry. Target slot identity and total inventory
  count were checked after each move. Five descriptions passed warning-removal
  checks. The source save hash remained unchanged.
- Long notes, page controls, download, cached completion, and the ready window
  were exercised. Actual rendered frames are retained under ignored
  `qa/menu-layout`; this supersedes the earlier black-capture limitation.
- Capture and inventory-test entry points are compiled only into the separate
  QA DLL. No capture detour, forced inactive rendering, spawned QA item, or test
  script is included in the runtime package.
- True hidden-Desktop D3D9 initialization is unsupported on this machine. QA did
  not activate or send input to the user's desktop. This is not a long-duration
  gameplay or fullscreen interaction certification.

Reference-only extracted game files are ignored under `analysis/soc-reference`.
Ordinary tests use five seconds after the required state, excluding loading.
The full deploy now contains eight managed files, including `ild_gameplay.script`.
