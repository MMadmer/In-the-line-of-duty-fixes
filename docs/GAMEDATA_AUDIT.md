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
the symptom, and the cause is the `ERROR:` line the log prints just before it. (The fifth pass reversed this:
the message is what players actually send, so the tail now names the reason.) Yura's `walker5` fallback timer
is commented out by the author and is a design decision, not a defect; the physical stalls it would have masked
are the watchdog's job. The 20 000-rouble exit charge is wired and reachable in the shipped files; the forum's
"the money is not taken" is not reproducible from the data.

## Fifth pass: crashes on entering a level, and scenes that never played out — 2026-09-10

Sources: two player reports on the mod's forum with the mod author replying, the reported crash text with the
`ERROR:` line a third player recovered from a full log, and mechanical sweeps of the effective script and
config set (mod overrides plus the vanilla tree the archives supply, plus the inline `custom_data` logic in
`all.spawn`). Each item was traced to the line that decides it before anything was changed.

### The masked crash message

`abort()` in `_g.script` ends with `printf("%s")` — a deliberately malformed format that turns a scripted
abort into a fatal. The fourth pass left it alone as vanilla behaviour. That was the wrong call in one
respect: it is also the reason every player report of this class is unusable, because the crash box shows
`_g.script:23: bad argument #2 to 'format'` and never the reason. The abort tail is now rewritten in memory,
same length and same line breaks, to `log("ERROR: " .. reason)` followed by `error("ERROR: "..reason,2)`. The
fatal is unchanged; the message names the object and the section. The reason is no longer put through a
second `string.format`, so a reason containing a `%` reaches the log intact.

### Repairs

| Defect | Consequence | Repair |
| --- | --- | --- |
| `space_okno_prizrakdoma.ltx:3` — `active = {-esc_dom_prizraka_kiknem} sr_idle` with no unconditional else, while every section of that same file switches to `nil` on exactly that portion | Once the ghost-house zombie has granted the portion, `determine_section_to_activate` picks nothing and aborts, on every later entry to Cordon, the moment the loading screen ends | `parse_condlist` rewrites that one `active` line to `{-esc_dom_prizraka_kiknem} sr_idle, nil`, the fallback vanilla writes for the same idiom |
| `cfg_get_npc_and_zone` aborts when the story object a restrictor watches no longer resolves. Six Cordon restrictors watch objects the mod itself releases: the two Garbage deserters (`038`), the BTR (`044`), the helicopter (`014`), the caged bandit (`043`), ATP Tikhon (`042`), the prison pair (`024`) | `ERROR: object 'esc_gar_dezertiru_sqda_zone': section 'sr_idle': field 'on_npc_in_zone': there is no object with story_id '038'` — a fatal on arrival at Cordon, reported by players and reproduced from the mod's own removal actions | The condition is kept so the numbered variants after it are still read, and given the engine's "no object" id, which the following lookup resolves to nothing. The `not in zone` form is neutralised the same way instead of firing on an object nobody can find. Every occurrence is written to the watchdog report |
| `smart_terrain.on_death` indexes the dying server object and its smart terrain with no check, while `unregister_npc` directly below it guards the second | A death callback that runs after a script already released the object raises inside the callback: `smart_terrain.script:1137: attempt to index local 'obj' (a nil value)`, reported for the Agroprom sniper after the toy-maker's conversation | The mod's own function is called only when both objects are there; otherwise the callback returns, as it would for an object that belongs to no smart terrain |
| `lab_psihoz113`, the X18 terminal carrying the "enter the password" tip, switches to `sr_idle2`; its own file declares the empty `ph_idle2` and nothing else | `activate_by_section` aborts on a section that does not exist, so entering the door code killed the game on the spot | The transition is rewritten to `ph_idle2`, the section the object declares for exactly that purpose |
| `tainik_s_gold_fish.ltx:6` plays `device\pda_news`; the file is `device\pda\pda_news`, which is how every other caller spells it | A `sound_object` on a name that exists nowhere is an engine fatal, so opening the Gold Fish stash killed the game | The mistyped name is aliased to the file it means, so the sound is heard rather than merely survived |
| The ATP scene is a ping-pong of remark sections between the spetsnaz leader (story 41) and Tikhon (42); the leader's last section is the only producer of `esc_atp_ydalai_ysex_k_xyiam_end`, and nothing in the chain has a timeout | An interrupted scene leaves the helicopter gone and Tikhon offering only a greeting: his closing dialog waits for a portion that will never come, and the whole line to Reba, Sidorovich and Agroprom stops there | A leader who is dead or released can never run his logic again, so the closing portion is delivered for him. A dead Tikhon hands the leader `atp_sdelka_t_fraza7`, the cue only Tikhon produces, and the leader plays the rest out through the mod's own condlists. With both alive and a section unchanged for 90 s, `combat_ignore` is re-enabled as a reload would, a wait for a sound that is no longer playing is released, and the section is asked to make its own transition |
| The dialog in which the actor reports both of Reba's spies dead is itself `esc_reba_proverky_proshel`, and its closing phrase grants `esc_reba_proverky_ne_proshel` — the fail portion of its own task. The completion portion the task names has no producer anywhere | «УБИЙЦА МЕСЯЦА» is stamped failed at the moment it is finished | The reward function of that one phrase runs nowhere else and tells the report apart from the two dialogs that legitimately fail the job; the completion portion is granted and both objectives that name it are settled. The fail portion stays: it starts the artefact task, spawns the pit artefact, and Volk and Sidorovich each have a dialog waiting for it |
| `absolqtno_drygoi_mamkin_shpion.ltx:15` and `psi_ystroistvo.ltx:72` run an effect from a timer with no target section, so the section never switches and the effect is re-applied on every update | Reba's spy re-declares war every frame from the moment he comes online; the psi device stacks a fresh post-process effector every frame through its blackout, because `run_postprocess` draws a new random id per call | `actor_enemy` is a no-op once the relation already says enemy, and an identical post-process is not re-added within half a second. Both do exactly what one call would have done |
| The dossier the actor takes off the Agroprom nationalist terrorist is declared with its own PDA article but named only in a `[known_info]` section of his logic file, which `xr_info` reads from a spawn ini and never from a logic one | Karabin's nineteen-phrase reveal about the shootout could never open | The portion follows the death portion of the same NPC, which also repairs a save in which he is already dead |
| `gulag_escape.ltx:2133` still targets `camper@esc_stalker_fox` after the mod renamed that section | Latent: nothing reaches `remark@esc_stalker_fox` today, but a target that does not exist is a fatal the moment something does | The transition is rewritten to `camper@esc_stalker_foxik` |

### Swept and clean

Every `active` condlist in the mod — 864 loose ltx, the merged gulag inis and the inline `all.spawn` logic —
was checked for a missing unconditional clause: the ghost-house window is the only one that can pick nothing.
Every scheme transition was checked against the sections its own file declares; apart from the X18 terminal
and the Lis camper above, the remaining hits are in files no spawn or script references. Every `=function`
and `{=function}` in every condlist was checked against the mod's `xr_effects` and `xr_conditions`: the only
misses are the Dead City hunt files, which nothing wires up. Every sound theme, and every file named by a
theme, by `snd =` or by `play_snd(...)`, was checked against the loose tree and all fourteen archives: the
only miss is the Gold Fish stash above. The eleven `arena_*` themes the mod dropped when it rewrote
`sound_theme.script` are reachable only through `bar_arena_start`, which nothing grants. The
`esc_atp_sdelka_*` themes and every file behind them are present, so the ATP stall is not a missing-sound
stall.

### Not changed, and why

The ransom branch of `esc_l_d_reba_start` is also the spy job's fail flag by construction: the portion it
grants carries the «ДОБРО НЫНЧЕ ДОРОГОЕ» task and the pit artefact, and two further dialogs wait for it. That
branch not granting `esc_l_d_reba_start` leaves Reba's opening dialog re-openable, so a player who refuses and
later accepts is handed a job that is already failed. Repairing that means deciding whether the two branches
were meant to be exclusive, which the shipped files do not say, so it is recorded rather than guessed at.

The logless crash on approaching the psi installation with the spy job active is not closed. The
discriminator is proven — `esc_absolqtno_drygoi_mamkin_shpion` is created 163 m from Reba, beyond the 150 m
switch distance, so its offline-to-online transition happens exactly on that approach — and every asset,
section, profile, visual, patrol path and graph vertex on that path was verified present and consistent, but
the engine fault itself was not identified. The per-frame `actor_enemy` storm that object produced is
repaired above; it is the one mechanism on that path that could be reached from script.

## Sixth pass: every level after the Garbage — 2026-09-10

Two player reports (the Agroprom military branch dead-ending, and the underground soldiers frozen in the
mind-controlled pose) opened a full sweep of Bar, Wild Territory, Dark Valley, X18, Yantar, X16, Military
Warehouses, Radar, Pripyat, the CNPP endgame and Dead City. The mechanical sweeps of the fifth pass were
re-run mod-wide first — dangling switch targets, `active` condlists with no unconditional clause, undefined
condlist functions, undeclared sound themes and missing sound files, story ids used by `on_npc_*_in_zone` —
and came back clean outside what is listed here. What the earlier passes did not cover is the class this pass
is about: **a scene section with no timeout whose one exit is a signal that may never arrive.** The mod is
built almost entirely out of them.

### Repairs

| Defect | Consequence | Repair |
| --- | --- | --- |
| The three Agroprom underground soldiers enter `[remark@suicide]` / `[remark66]` after the betrayal and nothing in the mod ever kills them, although Mozar's own lines say the controller did | They hold the `psy_pain` animation for good, and the six-man surface garrison never leaves its idle sections because every one of them waits behind `{+und_prapor_dead}`, which only the warrant officer's `[death]` grants. Killing the captain changes nothing | Each of the three is given the end its section is named for, on its own timer, the warrant officer first so the scream, the PDA line and the zombie wave still land in the room |
| `agro_door_open` carries no task, no article, no map spot and no tip, while the only way on is `nps_svalka.spawn_karabin` placing an NPC in an unmarked corner of the Garbage whose dialog is the sole grant of the portion that spawns the entire Bar | The branch ends with an empty PDA and nothing to act on | One PDA line, once. With the depot hint of the tenth pass, one of the only two texts the pack adds |
| `stroi_kom_logic.ltx:19` — the Bar formation's `[remark2]` leaves only on `on_signal = sound_end`, and `bar_krik_konec` has one producer and one consumer | Every quest of the rest of the game hangs on that one signal, and the commander is an ordinary stalker who can also simply die | A timer floor inside the section, the actor-side unfreeze below, and a last resort that grants the portion when no such NPC is left |
| `dt_ss_desantnik_kom.ltx:18` and `:56` — the SS paratroop commander's two remark sections have the same shape, and he is untalkable in every section but the last, which is where the ending dialog lives | An interrupted approach strands the ending | The same timer floors, on his own transitions |
| `bar_letiagin_final2.ltx:18,30` — `disable_ui` is undone by a single `on_actor_dist_ge_nvis` on the walk that follows, while a restrictor teleports the actor back to the same spot until that walk reports in | A walk that cannot be built leaves the player with no HUD, no input and no way to move | After a grace period the pack runs the section's own line: the stash, the portion, the input |
| `bar_deaktiv_iaderki` has one source, Melisa, and all seven finale devices test for it | Losing her ends the game with no way to finish | If she is gone, the item is handed over and her dialog counts as done |
| `val_underground_door.ltx:2` — the X18 entrance door was changed from `ph_door@locked` to `ph_door@open`, leaving `[ph_door@locked]` unreachable; its `on_use` is the only producer of `val_x18_door_open` and `dar_run_quest` | The lab task never completes, the Barman never takes the documents, the X16 task is never given, his stock never improves. Every playthrough | Reaching the lab, or holding the two keys the door asked for, grants both. The door is left open: locking it again would seal in a player who is already inside |
| `yan_grate.ltx:2,10` — the X16 grate starts fallen and its reopen waits for `art_start`, which nothing grants | A door vanilla holds open is welded shut | The vanilla condition is restored: it opens on `yan_labx16_switcher_primary_off` |
| The X18 closing cutscene waits for the pseudogiant to leave the pit | A corpse never leaves. The finale task, the cutscene giant's removal and two locked doors all hang on it | Once the giant is dead or gone for a minute, the portion it was holding is delivered |
| The four Military Warehouse bloodsuckers were dropped from the spawn, but the two tasks that complete on their deaths and Ugrumy's whole post-hunt branch were not | Two tasks sit in the PDA for good | Walking the village past all three arming restrictors stands in for them |
| `gulag_military.checkStalker` was re-skinned to `monolith` and `vrag` while the job lists still name the freedom and dolg profiles | Ugrumy and the blockpost commander never receive the meet dialog that is their only way of being spoken to | The wrapper accepts the communities the job lists actually name |
| Lukash still hands out an order against a Duty group whose leader and zone guard are not placed | A task that can never be done, with no way to fail it either | Reported failed when its targets are provably absent |
| `sar_monolith_gen_main.ltx:21` — `sar_monolith_destroy` has one producer, a `sound_end`, and is the only route into the room where the game ends | The good ending is one lost signal away from being unreachable | Six generators down and the portion still missing delivers it |
| `bun_deactivate_radar.ltx:10,20` — the brain-scorcher blackout hides the HUD and disables input, and gives them back only after a sound, a movie and another sound, none of which has a timeout | No controls for the rest of the save | After a grace period the portions that were owed are granted and the input comes back. The dream is not replayed |
| The five control-room guards deliver their part of the door counter as `=inc_counter`, which the death watch cannot make good because it only harvests info portions | The door to the generator hall never opens, and the good ending is behind it | With all five dead or gone the counter is **set** to what five deaths are worth, never incremented, so it can neither run ahead nor count twice |
| `esc_reba_proverky_proshel`'s closing phrase grants the fail portion of its own task | «УБИЙЦА МЕСЯЦА» is stamped failed at the moment it is finished | Covered in the fifth pass; the objectives are settled from the reward function of that one phrase |
| `dialogs_darkvalley.xml:1384` gates Mazai's last dialog on `af_gemchyg`, an artefact with no section, whose spawn function is called from nowhere | The moonshine quest stops after the two gas cylinders | The gate becomes the step before it, which the player can reach |
| `aes_space_restrictor_timer` — the CNPP surge was commented out of the timer but its section kept running with no exit | A counter sits at zero on the HUD for the rest of the level, captioned with the raw string id the commented-out line left behind | An exit at zero, so the scheme takes its own statics down |
| `nemec_sniper1/2_v_kpss_logic.ltx:26` — `[kamp]`'s daytime return targets the section it is already in, which `switch_to_section` refuses | Both Dead City snipers abandon their posts permanently after the first nightfall | The transition names `camper`, which is what it meant |
| `blok_dlia_prileta_xyiota.ltx:12` restarts the air-raid sound every update for five and a half seconds | One sound played dozens of times | `play_snd` now ignores a repeat of the same path within half a second, like `run_postprocess` already did |

Alongside those, every scene NPC above is watched from the actor's side as well: a section that has not
changed for ninety seconds gets `combat_ignore` turned back on, a wait for a sound that is no longer playing
released, and its own transition retried — which is what reloading a save does, and nothing more. The mod
sets `combat_ignore_cond = always` on nearly every scripted NPC and never sets
`combat_ignore_keep_when_attacked`, so one stray round from the player disables it permanently and freezes
whatever scene that NPC was carrying.

### Examined and deliberately left alone

The Bar arena is unreachable in this mod — `bar_arena_start` has no producer, the manager NPC was deleted from
the spawn — so the eleven `arena_*` sound themes the mod dropped when it rewrote `sound_theme.script` are dead
code and the megaphone can never abort. Every vanilla Bar task, `dar_codedoor_1/2`, `sar_monolith_go`, the
`sar_monolith_destory` typo and `pripyat_task_1` are cut or vanilla content that gates nothing. The Dead City
`config\scripts\cit\*` set is unreachable: the mod's Dead City is a new level and neither spawn holds a single
`cit_` object. `agro_ygoli_kvest` has no string-table entries, no hand-out and no completion — a draft, and
invisible as long as nothing gives it. The OGSM zombified-monster mechanic creates sections that do not exist,
but neither of its triggers (`actor_set_zombied`, `af_transmut_8`) exists either. The nine `[spawner] cond =
never` objects do spawn, because a bare token parses as a section name rather than a condition — on Radar that
replaced a vanilla gate, but the mod has been played and balanced that way, so restoring the gate would be a
content decision rather than a repair. `lab_psihoz115` names a logic file that does not exist; the object
simply gets no logic, and the two candidate files in that folder both carry `inc_counter(mon_destroy_generator)`,
which would pre-advance the endgame — wiring it up would break more than it fixes.

### Not closed

The ransom branch of `esc_l_d_reba_start` is still also the spy job's fail flag, for the reason recorded in the
fifth pass. The logless crash on approaching the psi installation with the spy job active is still open: the
discriminator is proven and every asset on that path verified, but the engine fault itself is not identified.
The Duty base dialogs on Bar render as raw string ids, because the mod's own `stable_dialogs_bar.xml` shadows
the archived file and drops every `bar_dolg_*` entry; none of those dialogs gates anything, so this is
cosmetic, and repairing it means shipping a string table of our own.

### The contact counter and the small config files — 2026-09-10

The mod repainted `textures\ui\ui_hud.dds` with a round bezel, left the vanilla region table in the archives
untouched, and moved the whole minimap into the screen corner (`zone_map*.xml`: `background` from `7,6` to
`0,0`, `level_frame` from `15,20` to `0,-5`). The contact counter did not follow.

The counter is `static_pda_online` in `config\ui\maingame_16.xml`, inside `gamedata.dbb`. Its own plate,
region `ui_hud_map_counter`, is **entirely transparent** in the mod's atlas; the disc the player sees is
painted into the `ui_hud_map` region, centred at atlas `159.3,165.8`. Measured on screen with the bezel where
the mod leaves it, that disc is at `297,306` and the number at `293.5,333` — low and slightly left, which is
what the screenshots show. The counter is attached to the bezel, so moving the bezel moves both and can never
close the gap: the number itself has to move, and it moves in `maingame*.xml`.

Reaching that file needed the delivery fixed first. Config repairs rode on `CreateFileMappingA`, and the
engine only maps a file above a size threshold; everything smaller is read straight into a private buffer.
Every repair to a small file — `string_table_ui.xml`, `esc_last_day_kamp.ltx`, `z_zvyk_psi.ltx`,
`avtodroch.xml`, `string_table_tasks_escape.xml` — had been failing silently since it shipped. Both exported
`CLocatorAPI::r_open` overloads are detoured now (`reader_repairs.cpp`), pinned to the validated xrCore and
its whole-instruction prologues, and every reader is offered to the same size-and-digest gated repair.

An archived file arrives as a read-only view of the archive mapping (`MEM_MAPPED`, `PAGE_READONLY`), so it is
not rewritten in place. The repair copies it, patches the copy, keeps it for the process and repoints the
reader at it. That is only done for a reader that owns no memory of its own: for an entry stored uncompressed
the engine hands out a plain reader over the archive mapping, which neither frees nor unmaps what it was
given. A compressed entry arrives in a private buffer and takes the ordinary in-place path.

The counter moves to `105,153` on the widescreen layout, which puts the number within half a pixel of the
centre of the disc at 2560x1440, verified on a loaded save. The 4:3 layout is derived from the same atlas
measurement and moves to `135,148`; it is not verified on screen. The minimap itself is left exactly as the
mod ships it.

## Seventh pass: money nobody paid, offers that vanished, and an updater tied to one build — 2026-09-11

Sources: three player reports from the mod's forum thread (Tikhon's unpaid share, the Wild Territory rally, the
auto-update session that lost its settings), two reports from the maintainer (Bronevik's detector offer, single-use
shovels) and the maintainer's note that a streamer without the mod's own binaries never received an update after 1.0.1.

### Repairs

| Defect | Consequence | Repair |
| --- | --- | --- |
| `dialogs_escape.xml` `esc_tixon_atp_final` — Tikhon's "вот твоя доля" line has no action, and no function anywhere in the mod pays 250 000 (the largest grant is 75 000). His earlier line guarantees exactly that sum, and it is the price Reba names in all three branches | Since 1.0.2 made Reba actually charge the ransom, the Cordon line stops at a quarter million the game never gives; «ДОБРО НЫНЧЕ ДОРОГОЕ» objective 1 has no completion condition at all | The same hash-gated repair that wires Reba's charge inserts `<action>ild_script_repairs.tikhon_share</action>` on that line, paid for with the block's own indentation; the charge never goes in without it. The share is paid once (`ild_tikhon_share_paid` in the actor's pstor, which the unmodified mod saves and loads as is). A save past Tikhon's dialog is paid when Reba's check runs. Objective 1 completes while the player holds the sum. A Tikhon who dies after the ATP scene no longer strands the line: his dialog counts as said once he is gone for good |
| `dialogs_bar.xml` `bar_bronevik_pochini_detektor` — the only reply without 5000 roubles gives `bar_bronevik_ne_chini_detektor`, and the dialog is gated on that same portion; nothing clears it | One visit without money hides the detector inspection for the rest of the game | The gate and its only writer are blanked in memory, at the file's exact size. The paid inspection still closes the dialog through its own portion |
| `dialogs_darkvalley.xml` `val_chr_torgash4_start` — the same shape: turning down 800 roubles gives `val_chr_torgash4_pshel_nax`, the start dialog's own gate | The only source of the "Bogdan" artefact (`af_teleport`) is lost for good, whether the player refused or simply could not pay | Same repair; buying still closes the offer |
| `xr_effects.minys_lopata` — the only consumer of `item_lopata`, run from the timers of four grave stashes. The first timer branch wins whenever the shared roll allows it, so Garbage and Agroprom take the shovel every time and Cordon's two about half the time | Some shovels "last" and others vanish after one dig | The effect is a no-op, at the maintainer's request (a deliberate mechanic change). Stamina, sounds, loot and the grave closing for good are unchanged. The description's "хоть и не долговечна" is replaced where the engine reads it (`CInifile::r_string`, as for the knives): `system.ltx` and its includes are parsed before any hook of the pack exists, so a file repair cannot reach them |

### The Wild Territory rally

**Trucks that stop.** The three opponents (`kar_sopernik1-3`, story ids 609-611) run stock `ph_car`, written for armed
BTRs. Once a second it compares `self.object:position()` with the previous sample and, below 0.2 m, calls `stop_car`
and ends the scheme for good (`ph_car.script:1045-1067`). That position is written from physics only in
`CCar::VisualUpdate`, which runs from `UpdateCL`, and `CObject::UpdateCL` (XR_3DA `0x433620`) keeps an object in the
per-frame update only while it is drawn, within 30 m of the camera, or asks for it through `AlwaysTheCrow` - which
`CCar` (xrGame `0x10269020`) answers yes only for an active mounted weapon. Driving commands still reach an unseen
truck, so it starts, and within about two seconds the check parks it where it stands. A player who takes the
organizer at his word and floors it at the signal leaves all three at the line; one who watches them sees them race.
The mod's binaries are not involved: none of the injected hooks touch the car, script-action or object-update code.

- `CCar::AlwaysTheCrow` answers yes for every car where its 28-byte body matches the validated one: its first
  instruction becomes `mov eax, 1; ret`. The patch is applied before any car exists. A car keeps updating whether or
  not anyone looks at it, as an armed one always did, so positions, steering, the stuck check and the finish
  restrictor (`on_npc_in_zone` reads the same position) all see where the truck really is.
- Everywhere else, `ph_car.action_car.fast_update` skips the stuck check for five seconds after `start_car` (a truck
  in first gear gets no torque for up to about a second) and while `CurrentVel()` - which reads live physics - shows
  the car moving. A car that really cannot move is still parked by the scheme's own check.

**No way back.** The race is staged behind the level's own invisible walls, and the mod's organizer says so («Мы за картой»). The walls
are GSC geometry, not the mod's: `materials\fake` faces with the ActorObstacle flag in `level.cform` (the door stands
0.53 m on the playable side of a 20 m high double-sided quad from (-214.39, 10.66) to (-204.91, 27.01)), and
`level.ai` holds no node at the camp or the finish. Walk-graph analysis of the collision mesh leaves the arrival
point, the camp, the start and the finish with no way onto the playable side short of a 1.5 km trip under the far
southern wall.

| Defect | Consequence | Repair |
| --- | --- | --- |
| The door `dt_door_na_dorogy_smerti` runs `=ddt_tptator_na_trasy_ralli` on every use; nothing anywhere takes the actor back | Anyone who enters, races or not, is stuck behind the walls | Walking back onto the spot the door put him on, after having left it, and standing there for three seconds moves him to AI node 23123, 1.8 m from the door on the playable side. Only the player's own move triggers it, so a race in progress is never interrupted |
| The finish restrictor's only return, `ddt_tp_gg_nastart` → `dik_ter.ddt_tpt_rally_na_start`, lands in the start camp, which is also behind the walls | The race ends and the player cannot leave | Once the organizer's closing dialog (`dt_ralli_gg_win`/`_fail`) has been had, or five minutes after the finish if nobody talks to him, the player is moved to the same node, once per session; a player who comes back through the door afterwards leaves on foot |
| The return fires 3.5 s after the finish, almost always while the player is still at the wheel, and `CCar::VisualUpdate` rewrites the driver's position every frame | The teleport is lost; the player is left at the finish, 500 m from the organizer, with the car now locked | The return is withheld from a seated driver and delivered once he is out of the car, and only while he is inside the rally area |
| The same return pulls the actor to the camp from wherever he is | A race resolving while the player has left drags him back behind the walls | The return is skipped outside the rally area |
| `dt_ralli_vodila1` alone turns the bet into the countdown | A dead or absent driver strands a race the player has already paid for | The countdown portion is delivered 30 s after any of the three bets if he never gives it; the existing rule then starts the race 40 s after the countdown |


- **Dead watchers.** In `ild_script_repairs.script`, `check_scene_stalls` named `unfreeze_scene` and `check_x18_pit` named
  `story_server` above their `local function` declarations. Lua compiles such a name as a global, which is nil at run
  time, and the `pcall` around both swallowed the error on every tick: the actor-side scene watcher and the X18
  pseudogiant fallback shipped in 1.0.3 and never did anything. The helpers now sit above their first use.
  `tests/script_globals.lua` reads the compiler's own listing of every payload script and fails on any global lookup
  of a name the script declares as a module local; it fails on the 1.0.3 file and passes now.
- **The 200-local ceiling.** Lua 5.1 refuses a chunk with more than 200 active locals, and this batch took
  `ild_script_repairs.script` past it during development: the whole file, and with it every Lua repair, would not have
  loaded. Nothing like that shipped. The rally helpers now live in their own block, the file holds 180 module locals,
  and the same test fails when a script does not compile.
- **The updater was tied to one executable.** The console bridge that carries the update window, the added options and
  the support verbs was installed only when `XR_3DA.exe` and `ui_main_menu.script` matched their digests, so an
  installation without the mod's patched binaries never saw an update. The bridge now validates the command layout
  against the executable's own exports: `IConsole_Command`'s exported vtable must hold the exported `Status`, `Info`
  and `Save` in slots 2-4 with no code after them, and the exported constructor, run on a scratch buffer, must write
  the vtable at 0, the name at 4 and three flags at 8-10 and nothing beyond. The menu is bound by path and by its unique
  anchor. Verified in staging with an `XR_3DA.exe` whose digest was changed: identity `MISMATCH`, updater and options
  installed, menu bound.
- **The file reader hook was behind the same gate.** The `_read` import that binds the menu and the actor script and
  installs the knife and texture repairs is resolved by name and is now installed on every build; scripts are
  recognised by resolved path (a game started through a junction, a SUBST drive or a short name reports its executable
  under that name while every file resolves to the real location, which silently cost 1.0.3 its console-spam fix and
  every path-based config repair there).
- **Pop-ups for an unfamiliar build.** Every tweak that found another build showed a message box on each launch. They
  are report lines now; a skip on an unvalidated file says "skipped", a failure on the validated one says "FAILED".
- **Without the bridge, the Lua payload issued verbs the console did not know**, which the engine logs as an error per
  call (the radio refresh once a second near a radio). The bridge is looked up once through `get_string`, which answers
  nil for an unknown command, and every verb is skipped without it.

### The loader report

`loader-report.txt` gains a `[settings]` section (the stored options, and whether UAC file virtualization redirects this
folder: the executable requests no execution level, so in a protected folder everything it writes lands in the
player's VirtualStore, where an elevated session and the updater never look) and a `[runtime]` section filled in as
scripts load: whether the console-spam statement was removed, the abort reason revealed, the menu and the actor script
bound. The keyboard-name and preset tweaks now check a position-independent signature of their own code instead of a
six-byte prologue.

The session that lost its radio setting and got the console spam back after an automatic update is not reproducible
from the data. Both symptoms together mean the bridge and the `_g.script` patch were both absent in that session: the
loader not loaded (a proxy DLL that an antivirus removes after the updater rewrote it), a changed `ui_main_menu.script`
and `_g.script` (a reinstalled or patched mod), or files read through VirtualStore. 1.0.4 removes the digest gates and
writes exactly those facts into the report, which is what to ask the player for.
## Eighth pass: the Garbage soul quest, and objects placed twice — 2026-09-12

Source: the mod author's own account of the Порча/Душа side quest on the Garbage, checked against the data. Three
of his statements hold and three do not, and the check turned up one way to lose the quest outright.

**What the account got right.** The stash of the two stalkers really is placed at one of three random points
(`xr_effects.random_spawn_gar_tainik_dvyx_s_artami`), and the point he suspected of bad coordinates — the one in the
bus by the road — is the best-placed of the three. `level.ai` was recovered from `gamedata.db0` (uncompressed, at
offset 178,516,615, identified by its version / vertex-count / cell-size signature and matching the level GUID in
`level.gct`), its node packing solved against 164 ground-truth pairs from `game.graph`, and all three entries then
checked both ways: every `level_vertex_id` is inside the level's 382,663 vertices, every `game_vertex_id` is inside
the Garbage band 252-415, and `level.gct` maps each level vertex to exactly the game vertex the table declares. The
bus entry sits 1.30 m from its node horizontally and **1 cm** vertically — tighter than any of the mod's nine static
`gar_tainik_*` placements, one of which misses by 57 m. `af_soul` is in the heap night-respawner's pool, and the
Garbage does have exactly three artefact respawners (heaps, willow, swamp), all three wired from `all.spawn`.

**What it did not.** The ecologist-suited corpse is the one on the depot tower beside the PDA
(`grar_mertviak_na_bashne`, visual `actors\stalker_zombi\ekolog_zombi_zheltuy`), not the one under the slab; under
the slab there is no corpse at all, and the ghost wears `a\38_strashnui_plash_antigas`. The tower diary names no
stash: its one occurrence of "схрон" is the writer complaining that the artefact cannot be put into one. Those are
content observations, reported to the author and not changed here.

### Repairs

| Defect | Consequence | Repair |
| --- | --- | --- |
| The grave carries two diggable mounds 22 cm apart: `grar_gygbfghjk` from `all.spawn`, and `gar_zemlia_dlia_dyshi`, spawned on top of it by `decor.spawn_gar_mogila_prizrak_dkr1`. Both take the shovel and the `af_soul`, both destroy the pair, both show `tip_zakopati` — and only the first grants `gar_porchy_bolshe_na_davati` | Which one the crosshair takes is chance. Digging the script-spawned one destroys the `af_soul` the player came for and grants nothing: `gar_porchy_bolshe_na_davati` is the `infoportion_complete` of both objectives of `gar_v_poiskax_dyshi`, so the task stays in the PDA for good, and it is also the flag the curse restrictor tests — whose 30-second timer is already armed and never checks for the artefact, so a fresh `af_porcha` arrives within half a minute. Recovery needs another `af_soul` (the stash is spent; the heaps give one about one night in nine) and the other mound, which nothing distinguishes | `parse_condlist` gives the script-spawned mound the portion its twin grants. Whichever mound the player digs, the task closes and the curse ends |
| `grar_grsvecha` and `grar_grsvechae12e2` are the same candle box at the same point; only the second was given `[ph_idle0] on_info = {+gar_dialog_s_prizrakom}` | The candles can be lit before the ghost has ever been spoken to. The second conversation asks only for the candles, and both ghosts share one profile whose `start_dialog` is that conversation — so a player holding matches takes the task, buries the `af_soul` and closes the quest without ever receiving the curse it is about | The older box's `on_use` is given the same portion check. Both copies now wait for the conversation |
| `grar_porcha_perezagryzka_spacer` `[sr_idle2]` — the section is entered while the actor holds an `af_porcha`, but its 30-second timer tests only `-gar_porchy_bolshe_na_davati` before running `=remove_item(af_porcha) =mne_porchy_art`. `xr_effects.remove_item` is `amk.remove_item(db.actor:object(...))`, which returns false on nil and is not checked | An `af_porcha` put into a box inside that window is not the one removed, so the pair leaves it there and creates a second | The removal is given `=actor_has_item(af_porcha)`, the condition the section's own entry already uses, plus a fallback back to that entry so the guard is not re-evaluated every frame |
| `gar_tainik_s_artami.ltx:13` — the first clause of the lock, the one that consumes a lockpick and leaves the box shut, plays nothing. The mod's six other locks all answer a failed pick with `device\xrysti` | A lockpick disappears in silence on the one box the quest cannot be finished without, which reads as a dead box | The missing answer is added. How often the lock resists is untouched |

All four are `parse_condlist` rewrites on a whitespace-insensitive match of the exact line, scoped to the object by
section name (the mound and the stash are created from their own `system.ltx` sections, so they carry that section's
`custom_data` directly and have no `[logic] cfg` to match on) or by object name (the two `all.spawn` objects).

### Swept and clean

Every pair of `all.spawn` objects within 2 m of each other that both carry inline logic was compared, mod-wide: the
only near-identical pair whose text differs is the candle box above, and the rest are numbered NPC groups differing
by a patrol index. Every one of the 63 coordinate-carrying spawn calls in `gar_vsia_xyinia_svalki` was checked
against the `all.spawn` objects it lands among; the grave mound is the only one that lands on a twin of itself
rather than on a restrictor or a particle source.

### Examined and deliberately left alone

`gar_v_poiskax_dyshi` has no `<map_location_type>`, no `<object_story_id>` and no `<article>`, the tower PDA is a
plain `II_ATTCH` with no binding, tip, sound or journal entry, and the diary is 2,265 characters with two line breaks
in a panel that does not scroll. Those are the author's, and stay: the maintainer's instruction is that unmarked
locations for the PDA and the Душа are intended. The lock idiom is the author's throughout — with one
`math.random(100)` shared by a whole condlist, every one of the seven locks has a band of rolls that does nothing,
and repairing that is a balance change across all of them rather than a defect of this quest.
`decor.spawn_grar_kpk_doxlogo_ycha` and `nps_svalka.spawn_gar_mertvui_ekolog_mrazi` are dead code; wiring the first
up would place a second PDA on the tower, and the second names a profile that no `character_desc_*.xml` defines. The
candle box's `on_timer = 8000 | %+gar_prizrak_vtoroi_prixod%` has no target section, so it re-grants a portion every
frame for good — a no-op after the first grant, and the only section it could switch to is `nil`, which would turn
the box into an ordinary container. `gar_tornado_dop` is spawned on the second ghost and never removed, but its
`effective_radius` is 0 and its `hit_impulse_scale` 0.0. The ghost's `target = 5053` points at a treasure object,
which only turns his head. All the Garbage content hangs off one grant of `gar_vsia_xyinia_svalki` in a Cordon
dialog; that is how the mod gates the whole level and not a defect of this quest.

### A ceiling to know about

The payload's repair script is a single Lua chunk, and the engine wraps it in its own namespace header before
compiling, which leaves **182 top-level locals** of Lua 5.1's 200. `ild_script_repairs.script` has stood at 181 since
1.0.4, one below the ceiling.

That was found the hard way, and the way it fails is the point: at 183 the game does not report a compile error. The
namespace simply resolves to nil, and the first script that touches it dies instead - here `ild_gameplay.script:17:
attempt to index global 'ild_script_repairs' (a nil value)`, on the main menu, with nothing in the log to say why.
Desktop Lua 5.1 loads the same file without complaint (its own limit is 200 and there is no wrapper), so `luac -p`,
the unit tests and the packaging all pass a build the game cannot start. It was caught by launching the game.

Measured on the installed build: 181 locals loads, 182 loads, 183 does not; 3.5 KB of added comments changes nothing.
New repairs therefore go inside `install()`, next to the hook they belong to, where the function's own register
budget applies - not into the main chunk. `script_globals` now counts module locals and fails the build above 182,
and `tools/qa/Run-SmokeTest.ps1` starts the deployed game offscreen, reads the log back and restores `user.ltx`.
**Every payload change must be launched before it ships.**
## Tenth pass: two things players walked into — 2026-09-12

Source: a stream of the mod and the complaints under it. Neither of these is a guess about what could go wrong;
both are what happened to somebody.

### Repairs

| Defect | Consequence | Repair |
| --- | --- | --- |
| `dialogs_bar.xml`, dialog `b_nac_mex_bazar` — the mechanic's drinking talk. Its hub is phrase 4, and every branch comes back to it: 13 names 4, 23 names 4. The wish-granter branch does not. Phrase 17, the mechanic's answer about the wish-granter, names phrase **2** — the one-shot "here, take the vodka" reply, which carries `<precondition>bar_ckpint.vodka_have</precondition>` and the `give_vodka` action | The bottle is handed over at the start of that same conversation, so by the time the answer is given phrase 2 is filtered out and the branch has no reachable reply at all, with the talk window still open on it. A streamer crashed on exactly that click. Carrying a second bottle is no better: the branch takes it and pours another drink | One digit in memory, at the file's exact length: phrase 17 names the hub its siblings name. It rides on the repair that already owns this file, so the digest gate is unchanged |
| The depot ambush on the Garbage. Once `gar_depo_napadenie_nachalosi` starts the fight, `gar_depo_korol_krus` — the karlik — re-grants `gr_depo_krus_podgon` every ten seconds, and every grant spawns three more tushkans (`info_l02garbage.xml:84-88`). His `[death]` section grants `gr_depo_krus_podox`, which is the only thing that stops the cycle | Killing the karlik is the whole answer and the mod never says so, or marks him, or gives the fight a task. The wave is endless until the player works it out, and the thread reads it as a broken quest | One line, once, twenty seconds after the ambush starts and never once the karlik is dead: «Что-то тут нечисто. Как будто, мутантами кто-то управляет. Надо бы поискать вокруг.» It is the actor's own thought, in the shape the mod's own `news.script` uses, remembered in the actor's pstor rather than in a portion of our own |

The hint says where to look and not what to do, which is the line this project has held for added text: the fight,
the karlik and the reward stay exactly as the mod wrote them.

### The crash that was not ours

The same pass re-opened the logless Cordon crash on the approach to the psi installation, on a report that it
went away with 1.0.3 and came back with 1.0.4. The first reading — that 1.0.4's `CCar::AlwaysTheCrow` patch was a
second, native way to crash on that path — was wrong, and the revert it produced has been undone.

What actually closed it in 1.0.3 is `install_story_object_repair`: the abort behind that crash is
`ERROR: object 'esc_gar_dezertiru_sqda_zone': section 'sr_idle': field 'on_npc_in_zone': there is no object with
story_id '038'`, and it does reach the log — players were posting a tail of the log that started below it. That
repair is **byte for byte identical in 1.0.3 and 1.0.4**, so it did not regress, and the vehicle patch is back
where it was.

The lesson stands on its own: a report that names two versions is not proof that everything which changed between
them is a candidate. The first thing to do with one is to find what closed it, and check whether *that* moved.
