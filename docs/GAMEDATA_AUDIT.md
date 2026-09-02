# Gamedata audit and repairs — 2026-09-02

## Result and coverage

The supplied log contained 114 diagnostic lines: 44 invalid sound-comment
versions, 24 missing sound comments, 34 keyboard-name conversion failures,
seven obsolete preset commands, and five missing-bump fallbacks.

The final latest-save run produced **zero** `!`, Lua error, assertion/fatal-error
lines. It used R2 at 2560x1440, an isolated app-data tree, an offscreen window,
five seconds after actor readiness, and a normal engine quit. This is a measured
smoke-test result, not a claim that every possible quest history was played.

Static coverage included:

| Area | Coverage |
| --- | --- |
| Lua | All 101 loose scripts; syntax, definitions, callbacks, escapes and lifecycle contracts |
| LTX/XML | 1,159 LTX and 172 XML files, including archive fallback |
| Effective system graph | 214 files, 3,292 sections; no missing includes or invalid ordered inheritance |
| Registries | 2,713 infoportions, 623 dialogs, 968 profiles, 1,221 characters, 212 tasks, 475 articles |
| Dialog calls | 5,159 callback references and all 623 dialog graphs |
| Archives | 14 archives, 25,684 indexed virtual paths |
| Assets | 1,846 OGF, 2,981 DDS, 31 OMF, 34 SEQ / 626 referenced frames, 183 shaders |
| Audio | All 5,973 loose OGG headers and comments; no invalid numeric X-Ray metadata found |

X-Ray-specific C-style Lua comments and lenient XML constructs were accounted
for. Standard-parser complaints alone were not treated as game bugs.

## Native boundary repairs

| Boundary | Repair | Preservation guarantee |
| --- | --- | --- |
| Keyboard names | Convert DirectInput Unicode labels to CP1251 instead of the CRT C locale | Scan codes and bindings unchanged |
| Sound metadata | Supply a valid X-Ray comment when the file only has ordinary encoder tags or no metadata | Copy the sound object's existing volume, attenuation, AI distance and type; audio bytes unchanged |
| Stock presets | Normalize seven unsupported lines in six exact-hash stock presets, only through the CFG reader | Supported settings and interactive command diagnostics unchanged |
| Missing R2 bump lookups | Any absent `*_bump` / `*_bump#` texture resolves to the exact `ed_dummy_bump` / `ed_dummy_bump#` resource the renderer substitutes itself after logging | Same resource records, no invented surface detail |
| Tank OGF reader | Exclude one trailing zero byte from the logical chunk stream | Exact hash, path, reader type and size checked; mapped file bytes unchanged |

The audio adapter leaves genuine binary metadata and malformed binary comments
alone. It is not a log filter. The preset adapter does not intercept `Log`,
`Msg`, or arbitrary console commands. An independent negative test emitted both
`ild_audit_unknown_command` and interactive `rs_detail` errors as expected.

A full play session shows ten more missing bump names (barrel, rust, rja, old wood,
signs) than the five-second smoke did, so the alias now covers every missing bump
instead of a list; the renderer would have loaded the same dummy after its warning.

The tank's original chunk stream ends at byte 2,096,475; its file has one extra
byte. The supported reader's chunk scanner otherwise attempts another 8-byte
header. Runtime QA verified the adjusted reader length and identical backing
bytes. Destructor behavior was checked against the installed xrCore binary.

## Script and configuration repairs

- Actor save: the mod aborts with `packer` (a typo) whenever it saves while input
  is disabled. The repaired branch writes exactly the bytes of the mod's ordinary
  branch, a zero flag and no timestamp, so every save made with the fix pack loads
  in the unmodified mod (rule 4). The remaining disable-input time is simply not
  persisted. Loading still consumes a timestamp written by the first 0.9.1 build
  (numeric flag `1`) and gives it the actor-punch timeout of 30 game seconds.
- Monster environmental deaths no longer dereference a missing killer. Original
  event arguments, smart-terrain notification and corpse impulse are retained.
- Sleeping consumes the first available food, rather than calling a void native
  method on missing items or consuming three foods through an OR expression.
- Artifact respawn respects the native void return. New artifact registry
  entries are reconciled once per actor update for a batch of zone spawns.
- Detector fastcalls return explicit booleans, survive ordinary updates, clear
  stale markers when the slot is empty, and re-register after actor reloads.
- Hidden knife callbacks handle the actually slotted knife instead of retaining
  only the last overwritten handler.
- Item removal includes inventory index zero and counts actual server objects.
- Six vehicle-repair and four weapon-assembly handlers preflight the complete
  original recipe before any side effect. ZAZ bearings and Volga hoses now require
  their original quantities. Recipe costs and original success handlers are unchanged.
- The Zhiguli description now says one headlight, matching its original predicate
  and consumption; the recipe was not made more expensive.
- Credits invoked without an actor use the original credits sequence. The mod's
  loaded-actor callback remains intact.
- Two incorrectly escaped sound-theme paths are repaired.
- Two absent callback names are aliased to their unambiguous existing functions.
- The X18 giant's missing `mob_walker@6` target becomes existing `@5` only under
  the exact object, section, field and condition fingerprint. Inline damage
  triggers are preserved, including whitespace-normalized INI values.
- Duplicate mechanic phrase IDs are separated without losing either upgrade;
  identical duplicate Prince phrases are removed from the virtual XML view.
- The missing `+` in the late-Cordon camp condition is restored.
- Literal angle brackets in a skill requirement become CP1251 guillemets, so
  TinyXML no longer drops that part of the displayed text.
- The Agroprom info registry receives its missing closing tag inside existing
  trailing whitespace; all prior nodes and total byte length are retained.
- The psi sound's invalid `on_use = no_use` is removed from its exact virtual
  config, leaving sound playback and all non-use behavior active.

### Explicit handling of obsolete quest links

The shipped scripts contain no `sidor_nagrada_1_6`, `give_albom`, or early
`ot_menia_monetu_bqrera` implementation. Their exact, scoped XML action links
are removed. Sidor's implemented armor/ammunition rewards remain; the later
implemented coin exchange remains; no album item or unknown extra reward is
invented. These changes repair the shipped quest graph, not missing historical
content from another mod version.

The orphaned cache-dialog money callbacks are restored from its explicit price
of 3000 roubles, without attaching that dialog to any new NPC.

## Regression evidence

- Native unit suites: sound comment classification, texture aliases, config
  transformations, same-size script patches and console editor behavior.
- 126 mocked script checks; 1,044 recipe checks including execution of original
  handlers under a mock engine and verification of their actual removal lists.
- Eight real config files passed exact original-hash matching and same-size
  transformation into isolated QA outputs.
- The first 0.9.1 build wrote and re-read a disposable save with timed input
  (timestamp restored, following data readable, expiry completed). The current
  build no longer persists the timestamp; the byte transformation is unit-tested
  and `tests/audit_save_roundtrip.script` now expects a load without it. The source
  player's save tree was not modified.
- Menu startup without an actor, latest-save startup, and resource-boundary
  probes passed. The clean run and intentional negative diagnostic run have
  separate logs under ignored `qa/audit`.

## Non-destructive contract and limits

All original game/mod files remain untouched. Runtime corrections use checked
module identities, exact source hashes or narrow existing API contracts. The
package contains eleven project-owned files; audit data, extracted originals,
disposable saves and the separate QA DLL are excluded.

No placeholder assets were invented for 25 unresolved texture names, two OMF
references and one shader in unused-looking model templates with no direct
current config/script/spawn references. Four unreferenced audio files use
stereo or 48 kHz; they were recorded, not silently resampled. An orphan journal
spawn callback and retired Yuriy route have no current producer and were not
resurrected with guessed objects or coordinates. Details are retained in
`qa/audit-assets/REPORT.md` and `qa/audit-quests/callback-transition-resolution.md`.

Those static candidates are not proof of an error in a current playthrough.
Conversely, a five-second run cannot certify every future story branch or old
save. Newly encountered genuine diagnostics must still be investigated rather
than suppressed.

## Correction: hidden-slot knife callback

The first shipped version of the hidden-slot repair replaced
`hidden_slots.BkgrWnd.AddCallback` through the luabind class object and called the
saved C++ method from the override. luabind treats an inherited C++ method fetched
from a class object as a forced default call; `AddCallback` has no default
implementation, so the first `BkgrWnd:InitControls` raised
`LUA error: pure virtual function called` and the game aborted as soon as the
inventory opened. The audit smoke run never opened the inventory, so the defect was
not observed there.

The repair now overrides the Lua-defined `BkgrWnd:InitControls` and
`hidden_slots.init_btn`, pre-seeds `ClickBtn[1]` before the original loop registers
it, and captures the knife button while the loop runs. The handler also requires a
live server object before converting the knife, so a repeated double-click cannot
spawn a duplicate. Runtime QA still has no probe that opens the inventory window;
the mocked suite covers the registration order, and the in-game check is manual.
