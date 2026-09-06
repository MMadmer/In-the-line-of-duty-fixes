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

## Second audit pass: quests, NPC logic and mechanics

A fan-out audit covered dialog item/money transfer, callback resolution, quest stages and info portions,
`decor.script`, NPC schemes and gulag jobs, level logic and `xr_effects`/`xr_conditions`, items and crafting,
save state and per-frame cost, world modules, and gameplay formulas. Every candidate defect was then given to an
independent adversarial verifier that had to fail on two grounds - that the defect is real and player-reachable,
and that the repair is possible without touching original bytes, save layout, story, balance or content - before
it was accepted. Findings that survived and are now repaired:

| Defect | Consequence | Repair |
| --- | --- | --- |
| `dialogs_yantar.give_ecolog_outfit` grants `stalker_outfit1`, a texture path rather than an item section | Hard CTD ("Can't open section") on a silent auto-phrase the moment the player talks to Sakharov after `bar_rescue_research_done` | Replace the function; grant `ecolog_outfit`, the section the mod actually defines |
| `decor.spawn_und_tv` builds its position with `vector(99)` | luabind raises on a constructor that takes no arguments, aborting the `ag_prap_nach_dial` action chain: the television and eighteen further barracks decorations never spawn and the closing `<task>` is at risk | Replace the function; use the default constructor with the original coordinates and vertices |
| `pochinka.b_mne_mod_ekz58` hands out `outfit_exo_mod15` | The mod58 exoskeleton has no producer at all; two Bar modernisation routes consume the base suit, a rare material and the money and return the wrong armour | Replace the function; grant `outfit_exo_mod58` |
| `dialogs_bar.xml` phrase 505 calls `pochinka.b_mne_mod_skat7` | The material-5 SKAT branch charges 5000 RU and one `modern_material5` for the material-7 result; `outfit_skat_mod5` is unobtainable, which also strands two further upgrade branches | Same-size byte patch of one character, anchored on the unique `b_bronevik_modern_505<` text id so the legitimate material-7 phrase keeps its own reward |
| `pochinka.give_fort` always destroys a plain `wpn_fort` | The two added Fort upgrade tiers never consume the pistol being upgraded, and destroy an unrelated plain Fort if the player carries one | Defer the handover; each of the six reward actions that follows it in the same phrase consumes the pistol it is actually upgrading |
| `CTreasure:give_treasure` ignores its own `done` flag | Ten dialog actions grant stashes directly, bypassing `CTreasure:use`; `esc_secret_truck_goods` is wired to two of them, so the box is refilled, a second map spot is added and one spot is never removed | Wrap the method to return early when the stash is already granted; first-time behaviour is unchanged |
| `mob_remark` reads `target` with `utils.cfg_get_number` under a string default | A section without `target` yields `""` and `alife():story_object("")` raises a type error that kills the monster's scheme; `target = actor` degrades to story id 0 through `atof`, so scripted mobs never turn to face the player | Normalize the value in `set_scheme` to a number or nil, and look at the actor from `reset_scheme` when the section asked for it |

Original story, balance, rewards and prices are unchanged: each repair only makes the game do what its own
configuration already says. All seven are covered by unit tests with a mocked engine, and the byte patch was
additionally applied to the installed `dialogs_bar.xml` in an isolated output, changing exactly one byte at
offset 304807 with the file length preserved.

### Second batch: crashes, NPC logic and mechanics

| Defect | Consequence | Repair |
| --- | --- | --- |
| `ogsm_mutants.script:18,21` builds `sound_object` for `anomaly\flies` and `monsters\phantom\phantom_snork_death` at module scope; neither file exists in the loose tree or the archives | A missing sound is an engine fatal inside the constructor, and it runs the first time anything touches the namespace - which `bind_monster.script:68` does near the Agroprom parasite zombie | Same-size byte patch to `nil`; both use sites already test the value before playing it, so the effect stays and only the sound is silent. This one cannot be repaired from Lua: the module dies while loading |
| `music_emb` asks for three sounds that were never shipped: `new\ost_mgnovenia_17`, `weapons\pm\pm_shoot`, `music\trava_y_doma_obrez` | Four reachable dialog and info-portion actions on Cordon, Garbage, Dark Valley and Agroprom drop the player to the desktop | Replace `xr_sound.get_safe_sound_object` with a wrapper returning a silent stub for absent paths. It also covers the 41 `soundtrack\*` calls in `ogsm_mutants`, whose directory is absent entirely |
| `gulag_escape.ltx:225` gives the blockpost sniper's night sleeper the absolute path `esc_voen_sniper_spati` | Gulag jobs prefix paths with the smart terrain, so the engine asks for `esc_blokpost_esc_voen_sniper_spati`, which is not in `all.spawn`: a hard error at the first military checkpoint whenever the blockpost is alarmed at night | Wrap `xr_sleeper.set_scheme` and restore the absolute path for that one section, so the sniper still sleeps exactly where the author put him |
| `death_manager.script:22` whitelists communities by hand and omits `actor_freedom` and `trader` | `pairs(nil)` aborts the death callback for the six ATP spetsnaz and the two traders: engine callbacks are never unbound, the corpse gets no settle impulse and the NPC stays in the PDA ranking | Wrap `drop_manager:create_release_item` so an unlisted community completes the callback. The drop table itself is a file-local, so those communities still get no randomised loot - the abort is what is fixed |
| `stanok.script` assigns an absolute condition on repair, and the level-2 bench's slot-6 handler assigns `0.01` | The bench damages anything already in better shape than its tier, and the better bench destroys the outfit it is supposed to repair | Wrap the six handlers to apply `max(condition before, tier)`, so the tier is a floor and never a ceiling |
| `new_life.script` divides by 1100 and `amk.script:22` by 100 when computing `m_endTime` | `m_endTime` is compared against a seconds clock: five skill banners expire before their first frame and never appear, and the overweight icon expires far in the future and never goes away | Replace the six functions with the same body and the correct 1000 divisor, which the neighbouring banners in the same file already use |
| `decor.script:639` attaches Sidorovich's campfire light to game vertex 0 | That vertex is 619 m away in the digger tunnels, so alife never switches the light online at the bunker | Resolve the level vertex from the light's own position and take the game vertex from the actor |
| `xr_effects.esc_direction_fire` sends a tip id the mod's `string_table_tips_escape.xml` no longer defines | The PDA prints the raw key `esc_direction_fire` during the Cordon camp raid | Skip the tip when `game.translate_string` returns the id unchanged, which is how the engine reports a missing string |

## Examined and deliberately not changed

These are real observations, but repairing them would change balance, content or NPC design rather than fix a
defect, which section 1 of the working rules forbids. They are recorded here instead.

- **Carry weight.** The classic Shadow of Chernobyl complaint - a weight bonus that raises capacity but not the
  run threshold - does not apply to this build. `CCustomOutfit::Load` (xrGame.dll `sub_1024BAC0`) reads both
  `additional_inventory_weight` and `additional_inventory_weight2` into separate fields, `max_walk_weight` is
  loaded into `CActorCondition+0x138`, and the engine even exports `get_actor_max_walk_weight` to Lua. Eight of
  the nine sections that grant a bonus set the two keys so that the walk threshold is at least the carry bonus.
  The exception is `[outfit_stalker_m2]` in `misc/racya.ltx`, which grants +10 to the walk threshold and +20 to
  capacity, leaving a 10 kg band in which the player is loaded but cannot run. Its own comment describes it as
  a rucksack suit, so that may well be intended; changing either number changes carrying balance.
- **Blockpost yard patrol.** `gulag_escape.ltx:107` sleeps the guard whenever it is *not* evening, so he is
  asleep for twenty hours a day. The sibling sniper job uses the sensible `is_night`/`is_day` pair, which makes
  this look like a mistake, but swapping the conditions rewrites an NPC's schedule.
- **Missing Cordon tasks.** `tasks_escape.xml` was rewritten for the mod and dropped three stock `game_task`
  blocks that `info_portions.xml` still declares, so three accepted jobs produce no PDA entry. Restoring them
  means adding XML content, which no in-memory same-size patch can do.
- **`remont.script`** references six sounds under the absent `remkit\` directory, but nothing in the mod
  references the module, so no player can reach it.

## Third pass: NPC stalls that never time out — 2026-09-04

The complaint is the well-known one: an NPC takes a few steps and stands in a doorway, or walks to a stash,
sits down and never gets up, and the quest waits forever. It is not deterministic, it does not happen to
everyone, and reloading the save clears it. That last detail is the diagnosis.

Scripted NPC behaviour in this engine is entirely callback-driven, and none of the waits has a timeout:

- `move_mgr` sets `self.state = state_moving` in `setup_movement_by_patrol_path` and leaves it **only** from
  `waypoint_callback`, which the engine fires when the NPC reaches a patrol point. `move_mgr:update` does
  nothing but pick a walk/run animation. If the engine cannot build or complete the path — a blocked
  doorway, a corpse or a physics object on the only route, a vertex that is unreachable from where the NPC
  actually stands — no callback ever arrives and the scheme waits for the rest of the game.
- The wait that ends a section is armed by `state_mgr_animation`, which sets `callback.begin` only once
  `states.current_state == states.target_state`. An animation the state manager cannot reach therefore means
  `move_mgr:time_callback` is never called: the NPC arrives, adopts its idle or sitting state and stays in
  it. This is the "sat down at the stash" report.
- `move_mgr:sync_ok` clears a `syn` signal only for partners that died or went offline. A partner stalled by
  either fault above stays alive and online, so the whole team waits on it.

Reloading works because loading re-runs `reset_scheme`, which rebuilds the patrol from where the NPC now is.

### What was added

A watchdog in `ild_script_repairs.script`, hung on `xr_motivator.motivator_binder.update` and sampling each
NPC once every two seconds. Every recovery it performs is a call the game itself makes on its own recovery
paths, so no scripted step is ever skipped:

| Signature | Recovery |
| --- | --- |
| `move_mgr.state == 1` and the NPC has not covered 0.5 m in 20 s | `move_mgr:reset` with the scheme's own arguments — what `time_callback` does when a wait ends away from its waypoint. The second attempt also clears `last_index` and `current_point_index`, so the patrol is re-picked from the nearest point |
| The same after three attempts | Stand the NPC up, drop any animation holding it, and send it to the point it was walking to on a free level path - the engine's own `stalker_go_to_waypoint` idiom, planned from scratch and not bound to the patrol's edges. The scheme's patrol is restored the moment it arrives. This engine has no way to place a client NPC anywhere, so a detour is the only recovery available |
| `callback.func` and `callback.timeout` set with `callback.begin` still `nil` after 30 s | Drop the animation that never finished, then `move_mgr:time_callback` - what the armed wait would have called |
| `syn_signal` pending for 60 s | Issue the signal through `move_mgr:scheme_set_signal` |

### Why it cannot break working content

- A wait of `t=*` sets `pt_wait_time = nil`, which makes `state_manager:set_state` clear `callback.func`. The
  watchdog's arming check therefore never sees it, and a section that is meant to end on an info portion or
  a signal from elsewhere is never forced. That is the case where forcing would skip content, and it is
  excluded by construction rather than by a heuristic.
- Combat, danger, conversation, being wounded and a meet in progress (`meet_manager.state ~= nil`) all reset
  the timers: standing still is correct in all of them.
- The movement manager is only touched while it is running the active scheme's own path
  (`move_mgr.path_walk == storage[active_scheme].path_walk`), so a stale manager left over from a previous
  scheme is never acted on.
- A section change resets every timer, and the whole check runs inside `pcall`, so a stall check can never
  take the game down with it.

### Evidence

`tests/script_repairs_tests.lua` drives a fake binder, movement manager and state manager through each
signature: the grace period is respected, a moving NPC is never touched, combat, conversation and meet are
ignored, an armed wait and a `t=*` wait are left to the engine, and the escalation runs reset → reset →
placement in order. Each intervention is recorded through `ild_update watchdog` into
`.ild-fixes\runtime\npc-watchdog.txt`, capped at 200 lines a launch, which is also what a player sends when
reporting that an NPC still stalled.

## Fourth pass: quests that promised one thing and checked another — 2026-09-06

Sources: the 23 pages of the mod's forum thread, the mod's own dialog, task, info-portion and logic files, and
`all.spawn`. Each item below was traced to the exact line that decides it before anything was changed.

| Defect | Consequence | Repair |
| --- | --- | --- |
| `dialogs_escape.xml` dialog `esc_dengi_rebe` ("Вот деньги.") has no precondition and no charge, while its own lines, the task «ДОБРО НЫНЧЕ ДОРОГОЕ» and `new_life.don_reba_denga_za_artu_esti` / `ia_otdaq_rebe_dengy_250000` all describe 250 000 roubles; neither function is referenced anywhere | The artefact case is handed over for free | In-memory, same-size patch: the check becomes the dialog's `<precondition>`, the charge an `<action>` before the handover; the block's own indentation pays for the 116 inserted bytes (`config_repairs.cpp`, `add_ransom_check`) |
| `string_table_tasks_escape.xml` objectives `esc_kom_vodka_0/1` say "отыскать 15 бутылок водки / Принести водку старшине", but task `esc_dengi_jme` completes on `kom_door_open`, granted only by the 20 000-rouble `norm_vodka` dialog; the mod contains no check on any vodka count | The task tells the player to do something the game never checks | Same-size CP1251 rewording of both objectives to the money the sergeant asks for |
| `esc_orig_tainik_zakrut.ltx` guards its box with `on_use = {+aiaiaiai}` — the mod's "never" idiom (the portion is granted only by `new_life.smerti`, which kills the actor); Sidorovich's 12 000-rouble cache `esc_secret_truck_goods` (story 5018) is one of twelve boxes using it | The player pays and the box says "closed with a key" forever; a player in the thread confirmed it | Once a sealed box's treasure entry is `done`, `ph_idle.set_scheme` gives it the vanilla treasure box's `nonscript_usable`/`st_search_treasure`; a grant to an online box applies it at once. Boxes nobody was pointed at stay sealed |
| `dveri_tixona_podsobka.ltx` waits for `navuk_vzlom_lvl1`, which no file declares or grants; `avtpark_seif.ltx` has its only `on_use` commented out while its tip demands "Взломщик №1" | Two locks that can never open, both advertising a skill | Both burglar journals (`esc_jyrnal2_s_navuk`, `val_jyrnal_vzlom2`) stand in for level 1, as every other skill's level 1 is its journals: the door condition is rewritten in `parse_condlist`, the safe receives the commented line gated on the same condition |
| A scripted mob's `[death] on_info` portions are delivered only by the engine's death callback | A boar, psy-dog or tushkan that dies offline or falls out of the level never reports, and Yura, the Garbage forest quest or Sidorovich's rat count waits for good | Mobs whose logic promises unconditional death portions are registered at `net_spawn`; a server object found dead, or gone, delivers the portions still missing. A removal by any `delete.*` function, `xr_effects.remove_object`/`remove_obj_id`/`delme`/`delme2`, `dead_city.exterminate_nacsamlet` or `ogsm_mutants` is recognised as a removal, as is a server object already gone at `net_destroy` |
| Vanilla `mob_death:death_callback` writes `death.killer = -1` to a local declared only in the known-killer branch | A killer-less death raises inside the callback before the section's portions are given | The branch is replaced with one that records the unknown killer in the scheme storage and runs the section's transitions |

Examined and left alone: `abort()` in `_g.script` ends with `printf("%s")`, which is the vanilla engine's own
way of turning a scripted abort into a fatal error — the visible `_g.script:23 bad argument #2 to 'format'` is
the symptom, and the cause is the `ERROR:` line the log prints just before it. Yura's `walker5` fallback timer
is commented out by the author and is a design decision, not a defect; the physical stalls it would have masked
are the watchdog's job. The 20 000-rouble exit charge is wired and reachable in the shipped files; the forum's
"the money is not taken" is not reproducible from the data.
