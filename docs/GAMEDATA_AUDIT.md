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
  *Superseded in part by the thirteenth pass:* this looked only at the two limits. The stamina a step costs is
  a third use of weight, and walking measured it against the bare inventory maximum; that, and the courier
  suit's 25/30 pair this count missed, are repaired there.
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
- *Corrected in the thirteenth pass:* none of this covered an NPC that stands in move_mgr's walking state on
  purpose - a sniper on its post, a sleeper on its point - and those were reset and rescued once a minute. The
  schemes' own idle states are now recognised and every recovery stops after one round per section.

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
are the watchdog's job. (The eleventh pass reversed this too: the `;walker5` at `esc_qra2.ltx:82` leaves no way
out of `walker45` when a boar is lost, so `ild_quest_repairs.script` enters the author's own `[walker5]` after
150 s of Yura's own update time there - with live boars as well.) The 20 000-rouble exit charge is wired and reachable in the shipped files; the forum's
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

### The PDA map spots a player asked about

A second comment asked how to get the PDA map spots back, saying there had been none since the start of the game.
The spot machinery is intact and was checked end to end: `map_spots.xml` declares 37 spot types and every one of
them carries a `<level_map>` entry; `ui_common.dds` is the ordinary 1024x1024 atlas the rects are cut from;
`bind_stalker.script` still wires `callback.inventory_info` to `actor_binder:info_callback` and that still calls
`level_tasks.process_info_portion`, exactly as vanilla; `add_lchanger_location` is still called from the same
place, and the level-changer story ids it names are all present. Across the mod 115 tasks carry 125
`<map_location_type>` entries, so markers are not stripped mod-wide.

What is true is that the starting level is the thin one: `tasks_escape.xml` holds 26 tasks and only 8
`map_location_type` entries between them, and the mod's own side quests are mostly written without a marker -
the same decision recorded for the soul quest in the eighth pass. That is content, not a defect, and it is left
alone. Nothing was found that would remove every spot for every player, and it could not be reproduced here.

One real defect did come out of the search, in exactly that path:

| Defect | Consequence | Repair |
| --- | --- | --- |
| `escape_tasks.script:149-155` — the two Cordon spots the mod added to `process_info_portion` read `alife():story_object(093)` and use `obj.id` immediately. Every other spot call in that file and in `level_tasks.script` guards the lookup first, and story 93 is `Escape_stoim_dejyrim_mu_zone`, a restrictor the mod can release | `process_info_portion` runs inside the actor's info callback, where a raise is fatal, so a released restrictor turns granting `esc_post_pianka_tolik` or `ia_dejyrq_za_volka` into a crash | Both portions are checked for the object first and skipped when it is gone, which is what the guard every neighbouring call has would have decided. The rest of the chain is untouched and still sees every other portion |
### The detector job, finished

A player reported paying Bronevik five thousand for nothing. The author confirmed the shape of it: the chip "вроде
есть в файлах", he meant to hide it in the children's cache, and the branch never got written.

Both halves of that are true in the data. The job starts and stops like this:

| Step | State in the mod |
| --- | --- |
| Find the broken detector | Works. `gar_lomanui_detektor` is spawned in the Garbage swamp by `gar_scriptu.spawn_gar_bolota_detektor` at `(147, -1.8, -135.6)` |
| Pay Bronevik to look at it | Works. `bar_bronevik_pochini_detektor` charges 5 000 through `pochinka.b_killer_komb_give_dengy` |
| He names the part | Works, and it is the whole setup: "специальная военно-научная микросхема EVA-1400... Без этой схемы этот детектор - фуфло полное", ending on "если найдёшь микросхему - приноси, починю его тогда" |
| Find the chip | **Nothing.** `esc_mikro_sxema_koordinatu_tp` is declared in `quest_items.ltx:2917` and referenced by not one other line in the whole mod - no spawn, no stash, no dialog |
| Bring it back | **Nothing.** `bar_bronevik_pochini_detektor` is the portion the last phrase grants, and nothing anywhere waits for it |
| Get a working detector | **Nothing.** No dialog hands one over |

So the five thousand bought a sentence, and the portion the conversation grants is a dead end with no consumer.

### The repair

The pack finishes the branch along the line the author drew, and adds nothing beyond it.

- **The chip is placed where he meant to hide it.** (As 1.0.8 to 1.0.10 had it: the first time
  `agro_tainik_detei3` starts its scheme the chip is created inside it. That never happened - see the sixteenth
  pass: the cache is spawned by the diary on the tower, which nothing points at, and its custom data has no
  `[logic]`, so the box gets no binder and no scheme hook of the pack ever fires on it. Since 1.0.11 the chip is
  created server-side when the entry is given: inside the cache when it exists, otherwise at the spot the
  author's script puts the cache on.)
- **The hand-over is the conversation that was missing.** `ild_bronevik_detektor_gotov` is inserted into
  `dialogs_bar.xml` and into Bronevik's own topic list in `character_desc_bar.xml`, both at the files' exact
  length, paid for out of their indentation. It is gated on `bar_bronevik_pochini_detektor` - he has to have
  looked at the detector first - and on a precondition that tests for both halves in the rucksack. That is the
  whole gate: the topic appears only when the exchange can happen and disappears the moment it has, because the
  two items it needs are gone. No info portion of ours enters the registry.
- **The reward is the elite detector.** The broken one's own description says it is "лучший детектор аномалий в
  мире" and shows "абсолютно все аномалии", which is `detector_elite` exactly; no trader sells it, so the job
  stays the only way to hold one.
- **The map leads, one stage at a time.** The mod marks neither place. While the chip is still out there the
  cache carries a green spot; once it is in the rucksack that spot moves to Bronevik; once the detector is
  rebuilt both are taken down. The gate is the portion the inspection grants, so a save that already paid the
  five thousand is picked up exactly where it stands. The spots are read back from the map on every pass rather
  than remembered in a local, because the engine saves them with the game and a local would not survive a
  reload - so a reload cannot stack a second one.
- **Only the chip is asked for at the end.** He has already had the broken detector in his hands and been paid
  to look at it, and a player who did that before this repair existed may well have sold the useless thing on.
  Requiring it back would strand exactly the saves this is meant to rescue, so the hand-over takes the broken
  detector only if the player still carries it.
- **A cache that was already looted still yields the chip.** (1.0.8-1.0.10's claim; see above. Since 1.0.11 an
  old save gets the chip on its first tick, wherever the player stands, once, and the actor's pstor keeps its id.)
- **The two lines Bronevik and the actor say** carry their own CP1251 bytes, the way the depot hint and the
  Garbage PDA line do. A string file of the pack's own was tried first and never worked: SoC does not read the
  language folder, it opens the file names listed in `[string_table] files` in the archived `localization.ltx`,
  and a name that is not on that list is never opened. An id the table cannot find is handed back unchanged, so
  the text itself stands in for the id and the engine prints it as it is.

Nothing about the price, the broken detector, the cache or its other loot is changed.

### The detector job as a PDA entry - 1.0.6

The job had no entry of any kind: the portion its conversation grants is declared empty in
`info_l08rostok_bar.xml:326` and no `game_task` anywhere names it, so 1.0.5 led the player with two standalone
map spots. Those are now the job's own steps.

`tasks_bar.xml` gains the skeleton - a title and the entry's own objective - and nothing else, because neither
the children's cache nor Bronevik has a story id for the XML form of a map spot to point at. The two steps are
built in script with `SGameTaskObjective`, each carrying its own `green_location` and the object it belongs to,
which is what ties the spots to the task: the engine shows an objective's spot while it is open and takes it
down when it closes. Step one closes when the chip is in the rucksack, step two and the entry when Bronevik
hands the detector over.

The entry is given from script rather than from the portion, so a save that already paid the five thousand
receives it on the next update instead of never. The stage lives in the actor's pstor, so the entry is given
once and a reload does not repeat it.



## Eleventh pass: the mod's own forum thread, all 39 pages - 2026-09-12

Sources: every post in the mod's release thread (370 distinct player reports, with the author's own replies),
and a second static sweep of the gamedata in eighteen areas. What follows is only what a player can see.

### The author's cuts, and the rule this pass ended up with

The mod keeps the vanilla `all.spawn` and switches off what it does not want with `[spawner] cond = never`:
263 objects carry it - the whole vanilla population of Radar and Pripyat, the Military Warehouses factions, the
Bar's ecologist and Freedom scenes, the Garbage car-park battle (Dymok, his two men, the raiding bandits), the
Yantar scientists and Vasilyev's body, the Dark Valley captives, and on the Cordon Fox, the Guide and Shustry.
Two smart terrains are gone the same way, the Military-to-Radar door is parked at y = +130666, and the CNPP
surge line in `aes_space_restrictor_timer` is commented out in the spawn itself. On the forum the author calls
Dead City «путь в один конец». These are decisions, not defects: the pack does not restore a scene, an NPC, a
door or an item the author took out, even where a vanilla dialog that pointed at it was left behind.

Several candidate repairs from this pass fell to that rule and were withdrawn before release (see below).

### The reports that were already closed

Three of the loudest complaints in the thread were repaired in earlier passes and are recorded here so the
next reading of the thread does not reopen them. The knife/crowbar crash is the inventory detour of the sixth
pass, not a script fault: `CUIInventoryWnd::ToSlot` asks `GetSlotList` for slot 0, gets NULL and dereferences
it, and the pack's own detour is what keeps the swap alive. The console that erases whatever the player types
is the `printf` console line the script patch already blanks in `_g.script`. The Cordon-after-Agroprom crash
the author describes in the thread header is the story-object abort of the fifth pass. The corpse-search crash
on `ui_npc_u_st_bild_ex` that a player logged early in the thread is closed by the author's own later patch,
which declares that icon.

### Repairs

| Defect | Consequence | Repair |
| --- | --- | --- |
| `ph_door.script:28` takes the hinge of a model that has none and line 31 indexes it | Three objects wear door logic on a hingeless model - the table at Tikhon's and a table and a Lada body on the Bar autopark - and the level load ends in a Lua fatal players only escaped by changing `bin` | The scheme's own `reset_scheme` is replaced for exactly those objects with the original body over an inert joint, so the door reads as closed and cannot swing |
| `character_desc_escape.xml:3429` supplies `wpn_spas`, a section that exists nowhere | A trader's supplies are built inside `alife():create`, so the mod's own guide who walks out with Yegor is a hard crash 2.75 s after the dialog | The name is corrected to the shotgun the mod ships, paid for out of the next line's indentation |
| Six sound paths in `sound_theme.script` and `xr_giditara.script` name files that exist nowhere | Every one is an instant "Can't open wave file": the Duty siren, the checkpoint loudspeaker, the drunk commander, the hangar tape, the garage radio and four strums in eleven on the guitar | Three are aliased onto the file the author meant, two are dropped from their theme, and the guitar's roll is replaced with the range the folder actually holds |
| Yura's stash chain waits on kills and arrivals that a reload or a stray boar can make impossible | The most reported stall in the thread: he squats at the stash for good, or never speaks after the dogs | Deaths the chain missed are credited when the objects are provably gone, and the dig finishes on its own timer |
| `tasks_darkvalley.xml` leaves the moonshine task's own objective with an empty `<infoportion_complete>` | «Самогон для Мазая» never closes | The header takes the portion its own last step already grants |
| The mod's own monolith-suit errand grants `yan_find_scientist_semenov_start` | A task for an NPC the author removed lands in the PDA and never closes | The one grant is blanked; the errand's own portion, handover and spawn stay. (The sixteenth pass found the errand's own `monolit_have` and `monolit_done` declared nowhere, a fatal on the grant; both are declared now) |
| `dialogs_garbage.xml` lost the `<next>` to Abram's refusal | The answer numbered "2." in its own text can never be chosen, and the Garbage logic waiting on its portion never hears it | The reply goes back beside its sibling, paid for out of the dialog's indentation |
| `stanok.script:544` needs two part sections that are not defined, and three recipes' parts are placed nowhere | Three of the four level-2 recipes can never be built | The recipe takes the names `unique_items.ltx` really uses, and the orphan parts are seeded once into four stashes that already hold their siblings |
| `moa_agro.script:67` spawns a second extra instead of the mod's own `agro_nemo_tyt_vujivet` | "Nemo", his escape along his own path and his FN2000 are never seen, although his section, profile, logic and path all exist | The spawn names the section the logic, the path and the loot were written for. (Withdrawn in 1.0.11: Nemo is immune to fire, wounds and strikes, his remark scheme cannot leave while he has an enemy, and the actor and Karabin are both his enemies, so he stood in the shoot-out for good and Karabin never came to the talk. The mod's second mortal extra is back and old saves lose their Nemo once) |
| `bind_det_arts` builds its artefact registry once, never drops an id, and omits the expensive artefact of every family | Artefacts an anomaly drops in front of the player are invisible to every detector, recycled ids draw spots on crates, and the cheap detector beeps continuously | The registry is reconciled as the detector runs, stale ids are dropped, the five missing sections are added and the throttle is made a real one |
| `bar_ckpint.daq_iashik_nac_two` removes exactly ten crates | A player who carried more is left with up to seven undroppable 5 kg quest crates | The whole stack drains, and a save already carrying spares is cleaned up |
| The Radar antenna custom data raises the damage when a psy helmet is worn | The one field every playthrough crosses punishes the helmet the mod hands out for it | The helmet can no longer make the field worse than bare-headed |
| Three of the mod's own spawns fire twice and one `[remark]` carries `on_info` twice | Two zombies, two of each snork and four SS bodies on single points, doubled loot, and a surrender that never closes the shoot-him branch | One-shot guards on the script spawns, and the repeated key takes the numbered name the switch reader already reads |
| Raw identifiers on screen in four places | The Dark Valley toll gate reads as eight ids, the crate courier answers with one, the mechanic's second banner prints its own, and the 100 000 RU detector is named `det_art_super` | Three are size-neutral renames in the string tables; the detector's text is supplied at `CInifile::r_string`, because its file is parsed before any hook exists |
| `quest_items.ltx` gives the PP-4a sensor a pixel position for a grid cell and a one-letter name; `w_mp40.ltx` swaps the two German names | Both are carried by NPCs on four levels and sold by four traders | Vanilla's cell and the neighbouring string id come back, and each gun takes its own name |
| A «to Yantar» icon on a parked Rostok door, a jammer spot placed before the jammer exists, one bunker guard with no scheme, a crow voice per object id | A transition that is not there, a missing marker, a guard standing blank among four posed ones, and a slow sound leak | Parked changers lose their spot, the marker is retried until its object resolves, the guard gets the remark his neighbours carry, and the crow binder releases its voice |

### Withdrawn under the rule above

Each of these was fully specified and applied on this machine before the spawn was read against the author's
cuts; none of them ships.

- **The Yantar scientists.** Kruglov, Semenov and Vasilyev's body carry `cond = never`. A repair that created
  them again so Sakharov's vanilla task could finish would restore what the author removed; only the mod's own
  stray grant of the Semenov task is repaired.
- **A way out of Dead City.** The author's own `level.spawn` holds an unfinished exit stub and the parked
  Military-to-Radar record could have been moved onto it, but the author describes the level on the forum as a
  one-way road. The door stays absent.
- **Voronin's RG-6.** The cache object was converted into a fireball anomaly and the Freedom base it belonged
  to is switched off. The launcher is not put back; the vanilla task he still hands out stays as shipped.
- **Dymok's defence.** Dymok, his men and the raiding bandits are all switched off and their smart terrain is
  deleted; the task has no producer left. Nothing to repair.
- **Fox's camp and the Dark Valley captive.** Both community gates were changed by the author, and the vanilla
  NPCs behind them are switched off (the mod spawns its own Fox by script for its own story). The gates stay.
- **Sidorovich's jarred-anomaly errand.** Phrase 21 was unlinked and its hand-over function was never written:
  the author hid an unfinished side quest. Re-linking it would have exposed a dead end.
- **The Cordon guide's `doctor_meet` fatal.** The dialog that opens the deleted task belongs only to the
  vanilla Guide, who is switched off; the crash is unreachable.
- **The CNPP surge staging.** The `on_value` line is commented out in the author's own spawn record. The
  sixth pass gave the timer an exit; the sequence itself is not brought back.
- **The parasite swarm.** The hook matches the one section the author wired it for; extending it to other
  parasite spawns is a balance decision.
- **The stalker on the barrel.** `gar_ammo_5.45x39_fmj_0002` is switched off; his missing logic file changes
  nothing.

### Examined and deliberately not changed

The Bar night-mutant attack starts on its own - the `[kamp]` section is reached, so «СВЯТО МЕСТО ПУСТО» was
never blocked. The binoculars and the torch do come back from the fake item, so the hidden-slot path loses
nothing. Both were reported as defects and are not.

### Not closed

The save corruption after the poltergeist at the professor's is not repaired: the thread has the author
reproducing it on his own beta save with an empty log, and nothing in the shipped scripts on that path writes
a value the loader cannot read back. It needs a save that has already broken to be worth another pass.

The 302 vanilla Bar string ids the mod's own `stable_dialogs_bar.xml` shadows - Duty, the barman, the ecologists
and the arena - still print raw where those dialogs are reachable. Restoring them costs 45 830 bytes and the
whole loose language folder has 11 092 bytes of borrowable indentation, so no in-memory repair can carry them;
a string file of the pack's own would have to be named in the archived `localization.ltx`, which is possible but
would put a `text/rus` path back in the payload and strand every client below 1.0.8 again.


## Twelfth pass: pages 39 and 40 of the thread, and a fix from the thread - 2026-09-12

Sources: the two newest pages of the mod's release thread, and `PDA-map-fixes-for-ILD-1.0.5.zip`, which
uroboross2 posted there for use on top of the pack's 1.0.5.

### A regression of the pack's own

The loudest report on the two pages is «Забытые сокровища» announcing "task complete" every few seconds
after the controller appears, with a player noting it began with the pack's 1.0.4 and was absent on 1.0.2.
That is exactly right. The fourth pass gave the underground task a settle - both objectives re-asserted as
complete once `und_prapor_dead` and `und_prapor_3` are both held, so the engine's order of testing the fail
and completion portions cannot matter - and hung it on the death watch, which runs every three seconds. The
settle had no memory: every tick queued both objectives again, `set_task_state` announced each one again, and
the PDA repeated the line for the rest of the game. `settle_tasks` now reads the objective's state first and
never re-asserts one that is already complete; a failed one still is, which is the repair. The spy job's
settle was guarded already and is unaffected.

### Repairs

| Defect | Consequence | Repair |
| --- | --- | --- |
| The underground settle re-asserts completion on every death-watch tick | "task complete" repeats every few seconds after the Prapor dies, from 1.0.4 to 1.0.7 | An objective that already reads complete is left alone |
| `mil_ded_kontra.ltx` - `remark2` plays `mil_ded_kontra_start` and leaves only on its `sound_end`; `walker` is the one section with a meet; the theme is declared in `sound_theme.script` and not one of its files is on disk or in the archives | «Потеряшки»: the sniper is dead, Ded is found, and he sits mute for the rest of the save | An eight-second floor into `walker`, through the scene fallback table the fourth pass already keeps; a build that does ship the sound finishes it first |
| Every marker the mod places itself goes through a scan-and-`map_add_object_spot` function: the spot is not written into the save, and a scan that runs before its object exists places nothing | Markers gone after a reload, and the jammer marker absent until a much later conversation; the antenna stash, the ghost's station, the spy's stash, the BTR spot, the soap zone, the secret trader, Karabin and the two the info handlers place all behave the same way | Each of the mod's marker functions becomes a request; the request is honoured with `map_add_object_spot_ser` the moment its object exists, a spot an older version placed the volatile way is upgraded once per load, and every spot comes down on the portion that ends it. The list of markers and the serialised, retried spot are uroboross2's; the tenth pass's jammer-only repair is folded into it |

### What was taken from the community fix, and what was not

The archive holds three files. The idea and the eleven-rule list in `pda_quest_recovery.script` are sound and
were checked one by one against the mod: every target section exists (five of them in `misc/quest_items.ltx`,
which is why a search of `spawn_sections.ltx` alone misses them), every start and stop portion is declared, and
`mil_jora2_pred ydarom` really is spelled with the space. The two info-handler branches it bypasses in
`escape_tasks` and `garbage_tasks` do nothing but place a volatile spot on a story object they never check for,
so nothing is lost. What is not taken: the file name, which no released updater owns and which the pack's own
script contract has no room for; the `assert` on every marker function, which would have taken the pack's whole
installation down with it on a build that lacks one; and `map_spots_relations.xml`, which adds relation dots for
NPCs to the level map - a preference laid over the author's own file, not a repair. The third file is the
pack's 1.0.5 script with one added line, and needs nothing.

### Examined and deliberately not changed

- **The parcel box on the checkpoint** («коробка неактивная»). `kordon_new/korob_blokpost.ltx` makes the box
  usable only while the actor carries `sqjet_blokpost_vodka`, `vodka` or `vodka1`; the caption is shown before
  that. The player who reported it had no bottle. Another player on the same page describes the intended run.
- **The soap** («мыло не найти», «мешок не лутается»). The soap and its zones are real, spawned from
  `misc/quest_items.ltx` sections at the checkpoint; the bag with the rat is the author's joke. What players
  lost was the marker on the zone after a reload, which the marker repair above now keeps.
- **Yura on the Garbage** (the exoskeleton search) was a missed spot, answered on the page itself; the tunnel
  stall is the eleventh pass's repair.
- **A green bug on start** came with no log and nothing can be said about it.
- **Saves that stopped loading before the Dark Valley** are the open save-corruption item of the eleventh pass.


## Thirteenth pass: carried weight, a promise at the black market, and repairs that never ran - 2026-09-13

Sources: the maintainer's own list (the overweight drain, the courier suit, the black market extras, what
«Похмельняк» is for), a player's report of logless Cordon crashes, the crash dumps Windows kept on this machine and
the pack's own watchdog record.

### Walking stamina and the carry limit

The stamina a step costs is `walk_power + walk_weight_power * share`, and past a share of 1 the weight part is
multiplied by `overweight_walk_k` (5 in this mod), then by `accel_k` or `sprint_k`. What "share" is measured
against is the whole defect:

| Where | Share | Source |
| --- | --- | --- |
| Walking, `CActorCondition::UpdateCondition` (xrGame RVA `0x1DD060`) | rucksack weight over `CInventory::m_fMaxWeight` - the bare `[inventory] max_weight`, 60 | `movss xmm4,[esi+6Ch]` / `divss xmm4,[esi+68h]` at `0x1DD0EB`, then `ConditionWalk` at `0x1DD600` |
| Jumping, `CActor::g_cl_CheckControls` (`0x1CF456`) | the same weight over the owner's virtual at slot `0x98` | `CInventoryOwner::MaxCarryWeight` (`0x20E7F0`): `m_fMaxWeight` plus the worn suit's `m_additional_weight2` |
| The inventory window and the fatigue step of the same update | `MaxCarryWeight` | `InventoryUtilities::UpdateWeight`, `UpdateCondition` |

So a suit's capacity moved the number the player reads and the jump, but not walking: from the 60th kilogram on,
every step cost five times the weight share even inside a suit the inventory rated for 85 or 90. The one division is
now pointed at the virtual the jump already calls. The detour is validated by the whole moving branch and by the
jump code that proves what slot `0x98` is; on any other build it reports "skipped" and changes nothing. A unit test
enters the replacement the way the engine does, with every register loaded, and checks the share, that nothing but
`xmm4` changes, and that a missing capacity or a foreign inventory falls back to the original division.

Without a suit nothing changes, because capacity and the bare maximum are the same 60. Inside one, the weight share
below the limit is now the share of what the suit carries - the rule the jump always used.

### The courier suit

Nomad (`val_chr_torgash2`) sells `kyrier_outfit` for 30 000 with «что позволит таскать с собой килограммов под 90»
(`val_chr_torgash2_start_7`). `outfit.ltx:1641-1642` gives it `additional_inventory_weight = 30`, a walk limit of
90, and `additional_inventory_weight2 = 25`, a capacity of 85: the only unequal pair among the mod's own suits. Its
description is the dialog's text with «под 80», written while the base was vanilla's 50. The capacity is read as 30
and the description as «под 90»; no other item, trader or spawn names the section.

Its immunity section `[sect_kyrier_outfit_immunities]` (`outfit.ltx:1655-1664`) holds `1.00` for all nine hit
types, where the stalker suit it is cut from holds 0.01-0.035 and 0.00 for radiation and psi. `CCustomOutfit::Hit`
takes `hit_power * K` off the suit's condition, so one hit cost a third of the suit and a player's video showed it at
0% within minutes, radiation included. The nine values read as the stalker suit's while the file still says 1.00, and
a suit the pack meets in the actor's inventory below full condition - worn, on the belt or in the rucksack, at the
first run of 1.0.11 or when it comes out of a stash later - is set back to full once, remembered per suit id in the
actor's pstor (`ild_courier_restored_<id>`, `ild_mod_repairs.script`). A new suit is met at full, so its wear from
then on is its own and is never repaired again.

### The file repairs that never ran

`XR_3DA.exe`'s WinMain calls the settings loader (`0x40F830`, `system.ltx` and `game.ltx`) straight after
`Core._initialize` and long before input is created, which is when the pack's file hooks go in. Every file
`system.ltx` includes is therefore parsed from its unrepaired bytes - the shovel and the super detector were moved to
the ini getters for exactly that reason. Three eleventh-pass repairs were not: the PP-4a sensor in
`quest_items.ltx`, the German submachine gun names in `w_mp40.ltx` and the B-94 binding in `w_b94.ltx` shipped as
file repairs and changed nothing in the game.

They now correct the values where the engine reads them, and only while those still hold the mod's own:

| Value | Getters taken | Why all of them |
| --- | --- | --- |
| `kruglov_flash` `inv_name` «i», `wpn_mp40`/`wpn_mp40n`/`wpn_mp41` names | `CInifile::r_string`, both overloads | The MP-41 and the MP-40 variant inherit from the MP-40, so the section decides the name |
| `kruglov_flash` `inv_grid_x` 4000, `inv_grid_y` 1950 | `r_u32` and `r_float`, both overloads | The inventory cell, trade and PDA windows read the same key through four different getters |
| `kyrier_outfit` `additional_inventory_weight2` 25 | `r_float` | See above |
| `wpn_b94` `script_binding = bind_wpn.init` | `line_exist` | The binder asks whether the line exists before it looks the module up and reports the miss |

xrCore's shared-string overloads are thin jumps to the plain ones inside xrCore, so they never pass through the
game's own import of the plain getter and have to be taken separately. Every getter is an import resolved by name.

### A fatal on start: "bad node in heap"

One of the four dumps Windows kept is `mem_usage_impl` failing with `bad node in heap` inside `stat_memory`, which
WinMain runs by itself right after input is created. xrCore's copy of `_heapwalk` (`0x1001BAE0`) steps `HeapWalk`
once per call and checks the previous block with `HeapValidate`, all without the heap lock, so a block another
thread frees between two steps reads as a damaged node. It is the intermittent start crash Shadow of Chernobyl is
known for. xrCore's `HeapWalk` import now takes `HeapLock` on a walk's first step and releases it when the walk
ends, keeping the error code the walker reads; a heap that is really damaged still fails.

### The watchdog that stood people up

The pack's own NPC watchdog record on this machine held 151 lines, and most of them were one sniper:
`esc_soldat416020 camper@esc_blockpost_camper_day` - 68 resets, 27 rescues, 27 rejoins - with two sleepers, two
walkers and the intro con-man behind him. The third pass reads "move_mgr in state 1 and under half a metre in twenty
seconds" as a stall, but state 1 is also where an NPC stands by design, because move_mgr only leaves it on a point
with a look point (`move_mgr.script:240`, `:523-535`, `:593`). A sniper camper resets without a look path and holds
its end point (`xr_camper.script:109-115`, `:311`), and a one-point sleeper never touches move_mgr once asleep
(`xr_sleeper.script:63-69`, `:121-126`). So every such NPC was reset, then rescued - animation cut, stood up, walked
to its own point - reported "rejoined" two seconds later, and started over about once a minute for the whole session.

Idle by design is now recognised the way the schemes decide it, and nothing loops:

- **A patrol that ended on a terminal waypoint** move_mgr has already handled is idle; one whose callback was never
  delivered gets one reset, which delivers it.
- **A one-point sleeper** that has arrived is idle.
- **A sniper camper** on a flagged point of its walk path, by xr_camper's own test, gets `scantime_free` on top of
  the grace; on a terminal post the rule above holds it for good.
- **Every recovery converges.** A section gets one forced wait, one rescue and one re-plan; an NPC that stands again
  after that is reported once as `stuck` and left to its scheme until its section changes.

The tests' fake `distance_to` had been declared without `self`, so every distance was zero and the old "rejoined"
check passed without the NPC ever arriving; the new cases fail against the old script.

### The prison at the mill, after the first job

Reported from a fresh game: the artefact job done, back in the cell, the guard's call comes - «Эй вы, спящие! А-ну жопы
подняли и на выход! Работать будете.» - two prisoners walk out and "keep standing up into the captive pose", and
nothing moves on; the player can wander every level with an empty rucksack. The player's own autosave says where
the chain stands: `esc_bandit_zabiraet_dryga`, `es_meln_jegoj_vushel` and `esc_jegoj_na_vuxode_iz_konclaeria_nam_skazal`
are held, `esc_zakorpat_posle_jegoja` is not.

The chain, from the mod's own files:

| Step | What runs |
| --- | --- |
| Zakorpat's talk after the artefact (`esc_zakorpat_post_arta`) | spawns the escort bandit (`kor_soldiers.spawn_esc_m_bandit_za_jegoj`), who walks to the cell and, within 10 m of the actor, grants the call (`meln_band_za_jegoj.ltx:22`) |
| The call | Jegoj `sleeper → walker2` (`jegoj.ltx:26`) and the novice `sleeper → walker` (`novic_aptechka.ltx:24`): both walk to the gate, into `def_state_standing = prisoner`, and their next transitions are commented out (`jegoj.ltx:44`, `novic_aptechka.ltx:41`). The camp door closes (`meln_konclager_dver.ltx:31`) |
| 22 s later | Jegoj grants `es_meln_jegoj_vushel` and the "leave" voice line; the door opens for good (`meln_konclager_dver.ltx:37`) |
| **Zakorpat's dialog `esc_zakorpat_posle_jegoja`** - «Свобода!» / «Ещё пока нет...» / «В сталкерский лагерь... Готовсь!» | the only thing gated on that portion (`dialogs_escape.xml:6671`). It also spawns Tikhon's doors and the big door for later, and moves Zakorpat onto Jegoj's mattress to wait for night (`zakorpat.ltx:27`, `:42`) |
| Night (0-7 h) | Zakorpat walks to the exit, 12 s, `esc_zakorpat_na_vuxod_info`, the talk «Через крышу?», a fade, the teleport out, Zakorpat 2 spawned outside, the old one and the two at the gate released on `esc_zakorpat_po_doroge` |

Nothing is broken in it: every path, portion, action and function resolves. The two at the gate stand there by
design until the actor's escape releases them. What the mod never does is say that the scene now waits on
Zakorpat - and it has just told the player the opposite. His previous line ends «В полночь я покажу, как отсюда
сбежать легко», so the player waits for midnight, and the midnight walk sits behind the dialog nobody was pointed at.
The "endless" standing up was the pack's own watchdog of 1.0.8 resetting the two every minute, repaired above.

The repair is the tenth pass's, again: one line, once, half a minute after the two are out and never after that
dialog has been had, shown for the ten seconds the mod's own tips last and remembered in the actor's pstor - «Ребят увели,
а ворота не заперли. Закорпат обещал показать, как отсюда сбежать - надо с ним поговорить.» A save that already
holds `es_meln_jegoj_vushel` receives it on the next load, wherever the player has wandered to.

### One more page of the forum, and the torch that crashed on being equipped

Source: a long list of observations posted under the mod, Cordon to Agroprom, plus a second player confirming
the one crash on it. Everything below was read against the files and, where it mattered, the player's own save.

| Defect | Consequence | Repair |
| --- | --- | --- |
| `CUIInventoryWnd::ToSlot` (xrGame RVA `0x3BBF80`) owns drag-drop lists for the pistol, rifle and outfit slots only and leaves the grenade slot alone by itself; for every other slot it moves the item and then hands the cell to the list it does not have. The sixth pass repaired that for the knife. The mod's own fake torch carries the author's note that clicking the real torch «игра вылетала, как обычно» | Equipping a torch by hand from the rucksack or the belt is a crash; two players report it, one since 1.0.4, when a real torch first became reachable in the bag | The knife path of the detour serves every slot the window draws no list for (asked of `GetSlotList` itself, the slot in ECX and the window in EDX), with the slot entry at sixteen bytes a slot. A torch, PDA or detector is placed without `Activate` and without the activate-slot event, since it has no hands to be drawn into; the knife, binoculars and bolt are drawn as before. Verified on the player's save through the window's own entry: a second torch from the belt takes the slot, the first goes to the rucksack, the count is unchanged |
| `kyza_logic.ltx:52` - Kuzma's wait at the anomaly ends on `on_timer = 100000 \| %+esc_kyzma_2% kamp`, and `esc_kyzma_2` is the portion his brother's dialog closes on. The scream dialog itself is gated only on `esc_spawn_tryp`, so it can be had at the camp, a hundred metres from where the corpse then spawns; the corpse's own timer (`trup_logic.ltx:6`) grants `del_esc_trup_stalk`, the gate of the brother's dialog, and an offline corpse never ticks | A player who does not follow Kuzma at night, or does not talk within his hundred seconds of probing, loses the brother's story for good - the reported case exactly | The timer still sends him home but no longer grants the portion (in memory, at the file's length); `del_esc_trup_stalk` is granted two seconds after the spawn wherever the actor stands, and its own action removes the corpse as the timer would have. (Since 1.0.11 the same line is repaired where it is parsed, so a save that spawned Kuzma before 1.0.9 - whose logic lives in the save - gets it too, and a closing portion such a timer already granted is withdrawn while the scream has not happened) |
| `esc_zakorpat_po_doroge` is the only release of the prison's two extras (`delete.esc_del_chrez_zakorpata_jeoja`, `delet_esc_s4_apteky`), and that roadside talk can be walked past | The prisoner and the novice stand at the camp gate for the rest of the game, beside the Jegoj `esc_makarov_posle_prizraka` spawns for the player to free - the «двойник» of the report | Once that later portion is held, every `esc_jegoj` and `esc_novic_aptechka` object is released, once, remembered in the actor's pstor. The freed Jegoj (`jegoj_tehn2`) and the village one (`esc_jegoj3`) are matched by exact section and untouched |
| `toneli_smerti.ltx:25-26` - the death tunnel's re-entry section disables the input after 6.5 s and ends at 13 s in `nil` with no `enable_ui` anywhere | Standing in the tunnel's restrictor - it reaches the bridge and the bus stop - for seven seconds after the scene leaves the actor blind and paralysed for good | `=enable_ui` rides on the 13-second line with the tunnel's call, at the line's exact length. The fade on re-entry is the author's and stays |
| Every `ph_code` lock records an entered code only by switching its section, and `[logic] active` names the locked one | A lock opened and then unloaded with the level is a lock again, with the code unchanged - eight boxes across the mod, the report's stash among them | On the lock's first update the outcome its `on_code` condlist describes is replayed whenever every portion it grants is already held |
| `artefacts.ltx` gives `af_gravi` `inv_weight = 0.0`, the one weightless artefact of fifty-eight; `fake_lom`, the rucksack stand-in the hidden-slot script swaps in for the crowbar, weighs 0.3 kg against the crowbar's 3.5 | «Грави весит 0.00», and a crowbar that gains three kilograms when taken in hand | Vanilla's 0.5 for the artefact and the crowbar's own weight for its stand-in, where the engine reads them |

Examined and left as the author wrote them: the Toymaker on Agroprom is spawned by the end of the boiler-house
killer's talk (`agro_ybiica_start`), so shooting the killer before the talk is a choice with a cost (the
sixteenth pass corrects the rest of this note: the Toymaker's aggressive branch leads to the poltergeist story
and the room with the tower key and cassette 2, his peaceful branch led nowhere and locked that room for good,
which is repaired now); Yura on the Garbage is released by the author's `delme2` on `agr_mozar_start` only from
his first section `[walker]` (`gar_qra.ltx:14-22`), before the bandit's PDA is handed over, at his first online
update after Mozar's dialog, so the player finds him gone on returning to the Garbage - at that stage he has
given no PDA task, and the cost is the forest chain: the hatches never open, the tank camp and its journal stay
out of reach and `gg_navuk_krasnorechi_lvl1` cannot be earned; once the PDA is delivered Agroprom no longer
removes him; the drinking party's chain to the wagon is whole (the wagon
Tikhonovich, his path and the guard all spawn at the wagon, the timers are numbered timers vanilla supports) and
could not be made to fail from the files; the checkpoint safe's use area sits 0.7 m above the lock and cannot be
judged without the model on screen; the Mauser's `anim_empty = empty` names a motion its model lacks, but no code in
this binary reads `anim_empty`, so it is not what hides the gun; the two Cordon gas masks are two items with their
own weights; the sergeant's 20 000 goes through `dialogs.relocate_money`, which 1.0.2 verified takes the money;
the soul mound keeps its default caption for the three seconds between digging and the portion; and the teleport
into the tower is a puzzle the author hinted at rather than a broken destination.

### Updates across any number of versions

There never was a one-version rule. A client reads `releases?per_page=30`, takes the highest version of its own
major and goes to it in one step: the patch when it stands on the release listed just before the target, the full
archive otherwise, and the applier adds and drops files by the two manifests. Each released client was run with its
own binary against a loopback copy of today's list: 1.0.0-1.0.4 reach 1.0.8 in one step. 1.0.5, 1.0.6 and 1.0.7
cannot reach anything, because their installed ownership list names `text/rus/ild_fixes_text.xml` and their own
allowlist does not own it, so they refuse before touching a file; that is compiled into those three binaries, and
the one manual unpack the 1.0.8 notes ask for stays the only way off them.

What 1.0.9 changes is the part that could strand clients again:

- **A release is applied by its own updater.** The applier used to be a copy of the installed helper, so a later
  release that owns a new folder or reads its manifest differently would have been refused by every older client,
  in any number of steps. The helper inside the verified archive now does the applying; a patch that does not carry
  one keeps the installed helper, which is then the release's own. Every future helper keeps the `--apply`
  arguments and the `apply-result.txt` and `patch-rejected.txt` formats.
- **A patch is checked before the game closes.** The client presumes a patch was cut against the release listed
  before its target - the published 1.0.7 patch is based on 1.0.5, which is no longer listed. The downloaded patch
  is now held against the installation (its base and every file it leaves out) while the game runs, and one that
  does not fit is swapped for the full archive in the same session instead of after a failed restart.

`Test-UpdateJump.ps1` runs the real service through the mock API over a skipped release, a patch cut against a
withdrawn one, a fitting patch and an archive whose own helper must be the one started; `Test-ReleaseUpgrade.ps1`
now covers every earlier release and applies each a second time with the helper the candidate carries.

### Examined and deliberately not changed

- **The black market's silent people.** The market is the author's own, spawned by `dolina_manu.script` from a
  portion named `nedodelanui_chernui_runok` - "the unfinished black market". Its 52 NPCs resolve completely:
  profiles, dialogs, phrase texts, logic, sounds. The ones who only say «Привет» carry the template 227 of the mod's
  1159 profiles carry, `hai` plus the wounded-medkit dialog, and no dialog written for anyone there is left
  unattached. The traders trade, Nomad and the unnamed seller sell, the informant opens up as his quest arrives.
- **«Похмельняк»** is not an artefact to the engine: `gar_art_poxmelie` in `unique_items.ltx` is a vodka-class item
  that removes 0.05 of intoxication per use with a use count xrCore clamps to the largest integer, one copy, in the
  stash on top of the Garbage tank that Yura's hatch leads to. It works as written.
- **The toll-gate dialog** in the same market can never appear, because the portion it needs parks its guard in a
  no-talk walker. Re-entry never needs the pass, so it is a dead branch of the unfinished market.
- **Logless crashes on the Cordon**, reported with Wolf's newbie search active, Yura's checkpoint stash bought and
  Tikhon's computer used as a box. Only the newbie search carries a crash: it spawns the psi-antenna sound case
  `zvyk_psi_antenna` beside Tolik, whose theme names `characters_voice\scenario\yantar\psy_voices_1` in a Lua string
  that loses its backslashes, and whose `on_use = no_use` names a section that does not exist - the first kills the
  game when the case comes online, the second when it is used, and players read the second as a crash with no
  log. Both are repaired: the theme since 1.0.0, the case's logic since small config repairs reached the game in
  1.0.3 - for cases created after that; a case created earlier keeps its logic in the save, and since 1.0.11 the
  dead use is redirected where the condlist is parsed, so those saves are covered too. The logless crash itself
  is not closed by this: the player of #565176 reports it on 1.0.8, after both repairs, and it went away only
  on a fresh playthrough, so it stays open (see the sixteenth pass). The stash is information sold for 2 000 and a coded safe, and the computer is a looping sound box that
  stays openable; neither is released, moved or grows a save. A sweep of every loose logic file for switches to
  sections that do not exist found the psi case, the X18 pseudogiant's `mob_walker@6` the pack already redirects,
  and Yura's `walker7` on the Garbage, which waits on a portion nothing declares or grants - nothing new to repair.

## Fourteenth pass: the detector entry as a player's PDA showed it - 2026-09-14

A player on the latest release sent the PDA: the entry titled `ild_detector_task`, two steps reading
`ild_detector_task_1` and `ild_detector_task_2`, and no spot on the map at all, where 1.0.5 had shown one.

### What the engine does with a task

Read against the SoC sources of `GameTask.cpp`, `GametaskManager.cpp`, `map_manager.cpp` and `ui/UITaskItem.cpp`:

- A task is saved with its title, every step's text, spot type, object id and hint exactly as they were given,
  and the PDA translates title and step text through the string table only when it draws them. An entry given
  by 1.0.6 or 1.0.7 therefore keeps the ids those versions wrote for as long as the save lives, and 1.0.8's move
  to plain text reached only entries given after it. Since the mod's Bar string table is already patched in
  memory for Bronevik's two lines, the five ids go in beside them - title, two steps, two hints, 418 bytes out
  of the 881 bytes of borrowable indentation the file has - and every such save reads correctly at once.
- A saved task is rebuilt from the task file before the saved text is read: `SGameTaskKey::load` constructs
  `CGameTask(id)`, whose `Load` aborts with «game task id=» on an id the file does not declare. A save holding
  an entry under the pack's own id therefore cannot be loaded without the pack - which the README promises it
  can. New entries are hosted on the archived `game_tasks.xml`'s own `user_task`, a skeleton with one line, no
  condition and no step that only the cut user-spot feature ever gave: its title, line and icon are rewritten
  at give time and saved with the task, so the entry reads the same with the pack removed. The skeleton the
  pack adds to `tasks_bar.xml` stays for the saves of 1.0.6 to 1.0.9, which hold the old id; the actor's pstor
  records which id a save carries, and both stage changes and the hand-over address that one.
- A step's spot exists only as a map location the engine creates when the task is given and removes when the
  step closes; nothing puts it back once it is gone, and the PDA offers no button for a step without one. The
  spots are the pack's own serialised ones now, the way the mod's own markers already are since the twelfth
  pass: the stage decides which of the two places carries one, the other is kept clear, a spot a pass finds
  missing is put back, and the first pass of a session replaces whatever an older version left with one that
  carries this hint. The steps carry no location of the engine's, so nothing competes for the same spot.
- Neither the cache nor Bronevik has a story id, so both are found by section - a slice of 2048 ids per actor
  update rather than the whole simulation on one frame, resting ten seconds after a sweep that found nothing.

### Verified

Unit tests cover the new entry, the id kept by an old save, the stage-wise spots, a spot put back after it went
missing, a target that appears after the entry was given, and the size-neutral string table. On a loaded save
the string table answers for the five ids with the texts above, the entry is given under `user_task` with its
title and steps, the cache carries the green spot and Bronevik does not, and the chip in the rucksack closes the
first step and moves the spot to Bronevik.

## Fifteenth pass: pages 40 to 42 of the thread - 2026-09-14

Thirty-odd reports across the three pages, read against the files and against what the pack already ships.
Most were closed before this pass (the underground task's repeating notice, the torch crash, Yura's stash,
Ded's silence, the Jegoj copies, the tunnel's controls, the weights) or answered on the page as walkthrough.
Three are defects, one of them the pack's own.

### Repairs

| Defect | Consequence | Repair |
| --- | --- | --- |
| `agro_kotelnui_ybiica.ltx` `[remark]` grants `ara_tak_vtorogo_xyilu_ne_nado` the moment the killer raises his hands, and `[death]`/`[hit]` spawn the wounded killer at the stairs only while that portion is absent. The mod as shipped never granted it - the loader kept only the second `on_info` of the section - so the shoot-him branch stayed open forever; the eleventh pass read both keys and closed it at the surrender | A player who walks up (he surrenders) and shoots before the talk is done gets neither the wounded killer nor the Toymaker: the door to the cassette stays shut. Two players on page 41 describe exactly the branch that disappeared | The portion is granted with the talk (`{+agro_ybiica_start}`), the point where a second killer really is not needed. Twenty bytes, paid from the file's trailing blank lines |
| Nine doors and boxes are opened by a use that spends a key, a lockpick or a tool and switches the section (`lqk_na_vushky.ltx`, `gar_tainik_s_artami.ltx`, `dveri_oryjeika.ltx`, `seif_door_oryjeiki.ltx`, `oryjeiki_door_polki.ltx`, `dveri_v_derevne1.ltx`, `dveri_tixona_podsobka.ltx`, `zapor_kapot.ltx`, ...) while `[logic] active` names the locked section | The object comes back locked the next time it comes online, with the key gone - the tower hatch and the artefact stash for good - and the car hood yields its parts again after every reload | The section such a use led to is written into the actor's pstor under the object's name and replayed on the object's first update of the next session, the shape the code locks already use. Only a use that spends something, opens a `locked` door or ends in the author's `{+aiaiaiai}` idiom is remembered, and only when it lands in a section with no timer back - the stove lit with a match keeps burning down |
| `unique_items.ltx` declares the rat king `syper_art_dryg_tyshkan` as `[medkit]`, so it is a medkit to the engine, and the build's `use_medkit` key takes the first medkit in the rucksack | The key eats the artefact ahead of the ordinary medkits: minus health and a tushkan instead of a heal | Its `class` reads as `II_FOOD` where the engine reads it; the eat lines are the same for both classes and the tushkan comes from the script that watches the section |

The pack's own Letyagin safeguard waits three minutes instead of two before it runs the farewell's own line: the
walk ends 52 m from the actor, and a slow path deserves the room.

Verified: unit tests cover the killer's file at its exact size, the remembered and replayed lock sections, the
timed stove and the finished hood that are and are not remembered, and the rat king's class; on a loaded save
the door and box hooks are in place, and the scripts bind cleanly. (The rat king's class was read back through
`system_ini():r_string`, which is the text hook itself; the object factory reads the class through `r_clsid` in
the executable, which the pack did not take, so the key kept eating the artefact. The sixteenth pass takes that
import, and the check is the object's own `clsid()`.)

### Examined and deliberately not changed

- **The author's word that the pack cut his cutscenes.** Checked against the files: the rally countdown the
  pack falls back on after 40 s is a 4.5-second sound; the X18 finale portion is delivered only once the fight
  giant is dead or released for a full minute, and the cutscene giant stands until that portion arrives; the
  Letyagin release fires only when the farewell has not ended on its own after three minutes. (Two floors of the
  pack's own did cut a speech, the sixteenth pass measured them: the Bar formation's 25 s against a 54.05 s
  speech and the SS commander's 30 s against 31.99 s. Both floors are past the speech now.)
- **The X18 grate** («рубильник опустил, решётка не открылась»). The spawn's own logic: the far-room lever
  powers the grate button, and the shocking switch `labx_kakoito_ruchag2` - between the far room and the grate
  button, some fifty metres from the six-button panel - takes that power away again
  (`on_info2 = {+rybilnik_viebal} %-labx_knopka_reshetki_vkl% ph_button@vukl0`) while it powers the other
  door's button. Throwing both is the puzzle, not a defect.
- **The siren at Zhaba's base** never stops after `td_spawn_ad` because the author commented its `on_use` out
  for that state; the belt draws eighteen cells over `max_belt = 16`; the tools and the file's icon, the
  vertebra artefact's flight, the invisible wall on the Wild Territory road and the empty Mauser are the
  author's models and numbers.
- **Transitions «на кордон, на свалку» from the Bar** after the Warehouses BTR. Nothing in the loose files or
  the spawn moves the actor between levels except the level changers, none of which leads from the Bar to
  Cordon; the vanilla autojump is never granted. Not reproducible without the save.
- **Makarov's hand-in repeating after the safe** and Sidorovich's talk returning: both dialogs are gated on
  their own portion, granted on the last phrase; leaving either early is what keeps them available.
- **The checkpoint parcel box** is the twelfth pass's note again: it takes a bottle the player still carries.

## Sixteenth pass: pages 43 to 45 of the mod's thread and pages 2 to 4 of the pack's own - 2026-09-17

Every report of both threads was read against the files, the engine sources and the pack's own code; the
detector entry's missing spot, sent as a PDA screenshot, was traced to its root. The work order is kept outside
the repository (`qa/forum-2026-09-17/REPORT.md`). Fifty-five real defects, of which nineteen were the pack's
own; every one of them is repaired or, where the repair itself was the defect, withdrawn.

### The detector chain, as it really is

The map spot was missing because there was nothing to put it on. `agro_tainik_detei3` is spawned only by the
diary on the Agroprom tower (`info_l03agroprom.xml:119-128`), a side errand nothing points at; its custom data
(`tainik_detei3.ltx`) holds `[spawn]` and no `[logic]`, so `bind_physic_object.init` never binds it and the
`ph_idle.set_scheme` hook 1.0.8 to 1.0.10 waited for never fired - no chip was ever placed, for anyone, and step
1 was impossible in every save. Since 1.0.11 the chip is created server-side before the entry is given: inside
the cache when the diary's portion is held and the cache is found, otherwise loose at the very spot the author's
script puts the cache on (36.56, 3.86, -123.4, vertices 274164/451), so a cache spawned later stands beside it
and neither its supplies nor the chip is ever overwritten. The chip's id and the id of whatever carries the spot
are kept in the actor's pstor. Step 1 is keyed to that carrier at give time (`set_map_location`,
`set_object_id`, `set_map_hint`), step 2 to Bronevik in the task callback the moment step 1 closes - the one
place the engine hands its task object to Lua, and the callback has to be registered again on spawn because
`reinit` registers the mod's own before the pack's wrapper exists - so the PDA draws its arrow for a new entry.
An entry an older version gave keeps its unkeyed steps and is led by the pack's spots without the arrow. One
shared sweep finds the cache (only while the diary's portion says it can exist) and Bronevik, a slice of 4096
ids per 250 ms tick, resting longer after every empty pass; the 1.0.5 hand-over flag `ild_detector_done` reads
as stage 3; the first error of a session in any actor-side settle goes to the watchdog file.

### Repairs

| Defect | Consequence | Repair |
| --- | --- | --- |
| `ild_mod_repairs.script` 1.0.10 read `on_use` with `r_string` on every door and box use, and `CInifile::r_string` is a fatal on a missing key that no pcall can catch | F on any door or box whose section has no `on_use` - the code box in `[ph_idle@enable]`, the camp door, the factory door - killed the game: «Can't find variable on_use in [ph_idle@enable]» | `line_exist` first, which checks the section as well; the lock keys carry the level as well as the name, since the Cordon stash has namesakes on three levels, and a key 1.0.10 wrote under the bare name is followed only where this object's own lock leads to that section |
| `w_oc33.ltx:77` names `weapons\arsenal_shells1`, a particle Arsenal Mod shipped and this game's `particles.xr` does not have | Every shot of the OC-33 near the camera is `R_ASSERT3` "Particle effect or group doesn't exist" - a crash on either renderer | The value reads as `weapons\generic_shells` through the text hook, since `system.ltx` is parsed before any file hook exists |
| The rat king's class was corrected in `r_string`, but the object factory reads classes through `CInifile::r_clsid`, which calls `r_string` inside xrCore - never through the game's import | The 1.0.10 repair never took effect | Both `r_clsid` overloads are taken in the game module and, on the validated executable, in `XR_3DA.exe`'s import table; a corrected class goes through `TEXT2CLSID` |
| `absolqtno_drygoi_mamkin_shpion.ltx:21` locks the input on the spy's death, and only the inventory box spawned at his spot gives it back - an inventory box, online within 135 m of the actor only | A spy brought down further out, or shot from that far, leaves the player blind and paralysed; a reload gives the input back and the scene never happens | Eight seconds after the lock, an actor still locked and out of the box's reach is put in the room beside it, where the scene itself takes him, and the author's chain finishes; a minute later without the portion the input is given back |
| The X18 six-button panel: a press grants a portion for thirty milliseconds and the two ring neighbours read it from their scheduler-driven updates, thirty to forty milliseconds apart | A neighbour switches never, once or twice; the panel breaks its own rule of threes and the two solutions work one time in three | The press switches both neighbours itself, once, and withdraws the portion before any update can read it; a save whose panel is already out of the solvable class (odd counts in either parity set) goes back to the shipped position once |
| The death-tunnel safe's keypad case sits inside the safe's own model | The code is written on the safe and the place to enter it is unreachable to the crosshair from anywhere but a hand's width behind | A use of the safe without the code is handed to the keypad's own use; with the code, the safe opens as written |
| `agro_igra_final` phrases 662-669 (the eloquence line) grant only the empty `agro_ydivitilnui_cirk`; the door `agr_dver_y_parashi` opens on `agro_polter_sdox` alone, and the pack's 1.0.8 surrender repair closed the accidental way round | The peaceful branch locks the room with the tower key and cassette 2 for good - and through the key, the diary and the detector chip | The peaceful outcome grants `agro_polter_sdox`: the door opens by its own logic, the task of the other branch was never given, the fight is not started. 1.0.8/1.0.9 saves holding `ara_tak_vtorogo_xyilu_ne_nado` without `agro_ybiica_start` get the portion withdrawn (killer alive) or the double at the stairs (killer dead) |
| Nemo (see the eleventh pass's row) | Karabin's talk never opens | Withdrawn; old saves lose him once |
| `delete.delet_escape_volka` removes Volk when the story leaves the Cordon; `esc_poisk_novichkov_kvest` and `tolik_i_volk_kvest` have no end of their own | Both stay "in progress" for good | Closed with the removal - a served duty as completed, the rest failed - and once on load where the removal already happened |
| `dialogs_escape.xml:4153/4199`: Volk's and Yura's PDA dialogs share `new_life.esti_kpk_zakorpata` | Hand the PDA to Volk first and Yura's «тайник пуст» is unreachable, the stash chain with it | Each dialog points at a check of the pack's own (in memory, at the file's length): Volk takes the PDA only once Yura has read it, Yura's talk opens without it where Volk has it |
| Yura's logic restarts from `[logic] active` whenever he comes back online within a session | He walks off to the tunnel from wherever the chain had brought him | The latest stage his portions name is resumed instead |
| `esc_tainik_mamkinogo_shpiona`'s marker rule has no end portion | The circle stays on the Cordon map for good | It comes down once the box - spawned full - is empty, remembered in the pstor |
| `scene_fallbacks` floors of 25 s (Bar formation, speech 54.05 s) and 30 s (SS commander, 31.99 s) | The floor fired mid-speech and opened the next dialog | 70 s and 45 s |
| `dialogs_yantar.xml` grants `monolit_have` and `monolit_done`, declared in no portion file | A fatal on the grant | Both are declared before the closing tag the pack already restores in `info_l03agroprom.xml` (the Yantar file has 22 bytes of indentation to lend, the Agroprom one 161), paid out of that file's indentation |
| `tp_v_dp_logic.ltx` returns the actor from 73 m up after 4 s on the second throw; the fall takes 2.7 s | The actor hits the ground before the return | 2 s, like the first throw |
| `zvyk_psi_antenna`'s logic has no end | Its voices play at the bridge for the rest of the game | Silent once `esc_tola0_delet_i_1_spawn` is held; the case and the save stay as they are |
| `agro_door_open` has one source, the captain's talk after the underground; the forum's advice was to kill him for a key that does not exist | Doors shut and Karabin unspawned for good | A captain dead or gone after `und_prapor_3` has the portion granted for him, and the actor's community restored as his talk would |
| `treasure_manager.ltx:298` `esc_secret_stalker_things` names story object 5008, which the author removed with four trader stashes; `gar_secret_box_blockpost` holds the only guaranteed `item_avto_fara` of five the four car repairs need | The dead record eats the draw for nothing; the ZIL and the Kopeika, and the cars the barman sells after them, were a 0.75% chance | The record is granted into the box it was written for (`level_prefix_inventory_box_0012`, by position, since the name repeats across levels) and its spot comes down when the box is emptied; records without a box stay out of the draw; `gar_secret_bus_tube` and `gar_secret_deadman` carry one headlight each |
| `bar_gg_nivy_pochinil` is read by nothing | The Niva stays broken and its parts can be paid again | The bench stays shut once the portion is held |
| `delete.del_esc_kor_norm1` matches names containing "norm" | Dozens of unrelated objects across the Zone are released with the Cordon guards | Sections, the family for the first removal and the numbered guard for the rest |
| 81 custom-data files open with a UTF-8 BOM, which the ini reader takes as part of `[smart_terrains]` | 82 scripted NPCs read as free for any camp | The conditions are answered for exactly those sections when the reader found none |
| The tower hatch spends its key, and before 1.0.10 came back shut | The diary and the caches out of reach for good | A save with the decorations spawned, the diary unread, nothing remembered and no key anywhere gets the open section written once |
| The depot and prison hints count their delay on any level | The one line is spent elsewhere | Only on their level, the depot's near the karlik's lair |
| `inventory_new.xml:16` and `_16.xml:16` caption the belt "Belt"; the re-chambered guns inherit their parent's description; the mechanic says the PPS-43 takes 9x19; the file's line is captioned «Ножовка.» | Texts contradict the items | `ui_inv_belt` (the mod's table has «Пояс»); descriptions of the AK-47 (7.62x54), MP5 (.45), PM and Fort (9x19), HPSA (9x18), the sawn-off TOZ and the German Karabin-98 (7.92, and its name says so) through the text hook; the mechanic's line and the caption in memory at the files' length |
| `outfit.ltx` `[sect_kyrier_outfit_immunities]` holds `1.00` for all nine hit types; every other suit holds 0.01-0.05 and 0.00 for radiation and psi, and `CCustomOutfit::Hit` takes `hit_power * K` off the suit's condition | The courier suit lost a third of its condition to one hit and wore to 0% within minutes of play, radiation included - a player's video showed it | The nine values read as the stalker suit's, whose bone protection the courier already shares, while the file still says 1.00; through the number hook, since the file is parsed before any file hook exists. A suit the pack meets in the actor's inventory below full - at the first run of 1.0.11 or out of a stash later - is set back to full once, remembered per suit id in the actor's pstor (`ild_courier_restored_<id>`); a new suit is met at full, so wear after that is its own |
| `bind_gameplay` replaced the actor script's anchor whether or not the Lua payload was on disk | A patch unpacked without its base died at the actor's spawn: «attempt to index global 'ild_gameplay'» | The five scripts are checked at start; missing, the binding, the menu binding and the dialog repairs that name Lua functions stand down, the loader report says which file is missing under `[installation]`, and a message box names the archive to take |
| The helper made one unauthenticated API request and treated any failure as "nothing to offer", silently | Behind a shared address the hourly allowance is spent by others and updates never come; the player cannot even see the pack's version | Retries after the pause the API names, a fallback release list uploaded as a third asset of every release, an ETag-conditional request, `update-last.txt` copied into the loader report, the helper's start recorded, the pack version and the check's verdict in the main menu |
| The updater refused with 27 over a hand-modified `ild_*.script` and with 26 over pack files left by a manual rollback; `ild_fixes_text.xml` of 1.0.5-1.0.7 lingered | The forum hotfix blocked every update; an old archive over a new one blocked them too | The pack's own gamedata files are its own whichever list they are on; the retired text file is removed when its bytes are a released copy's; codes 26 and 27 name the file in Russian |

### Examined and deliberately not changed

- **The sleeping row in the Bar** (`nac_sold_logic.ltx`, `is_night_two` → `sleep`) and the corpse hanging by
  a door in the same screenshots: the author's schedule and the engine's physics.
- **The second spy in the wagon**, the graves on Agroprom, the third-floor doors, the BTR, the eloquence skill:
  the game does what the author wrote; the pinned FAQ of the pack's thread described each wrongly, and the
  corrected texts are in `docs/FORUM_FAQ_CORRECTIONS.md`.
- **The X18 grate button's power** is reversible, not lost for good, and the grate closes again when the
  shocking switch is thrown - the FAQ, not the puzzle.
- **The logless crash on the Cordon** (#565176) survived 1.0.8 and a rollback to 1.0.3 and went away on a fresh
  playthrough; the psi case's two repairs do not explain it. Not closed.
- **Ded in the Warehouses village**: the theme resolves to `tu_kto_takoi.ogg` (1.03 s) and normally ends on its
  own signal; the eight-second floor is a guard against a stall whose cause is not established, not a missing
  file, and the comment now says so.
- **The eloquence skill's price effect** is the mod's own reputation table, renamed; «Барыга» and «Лжец» are
  the author's unfinished work.
- **The mod's hidden-spot hints and «смехотворные» quests** the author objected to: the depot and prison lines
  are one sentence each, once, in the mod's own news shape; the detector entry is the author's own unfinished
  branch completed along his line, hosted since 1.0.10 on the game's own skeleton.

### Verified

Unit tests cover the strict ini reader, the level-qualified lock keys and the legacy ones, the panel's
neighbours and its normalisation, the safe's redirect, the hatch's reseed, the detector chain from the chip's
creation to the keyed steps and the callback, the spy's lock and its failsafe, the captain, the Volk jobs, the
PDA checks, Yura's resume, the marker's end, the stash alias, the headlights, the Niva, the norm removals, the
psi case, the BOM'd sections, the hints' level, every new config repair at the files' exact size and the Lua
gate of the dialog repairs, the split of `bind_gameplay`, and the updater's applier scenarios. The shipped
scripts are checked for staging notes and for modules that do not ship.
