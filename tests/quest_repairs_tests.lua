local source_path = arg[1] or "payload/gamedata/scripts/ild_quest_repairs.script"
local tests = 0

local function check(value, message)
    assert(value, message)
    tests = tests + 1
end

local function equal(actual, expected, message)
    check(actual == expected, message .. ": expected " .. tostring(expected) .. ", got " .. tostring(actual))
end

-- The module is loaded into an environment of its own, the way the engine compiles each script inside its own
-- namespace. Nothing here talks to a real game: every global the repairs reach for is a stub that records what
-- it was asked to do, so a repair that stops doing it fails the test.
local function fixture()
    local calls = {console = {}, logged = {}, created = {}, spots = {}, infos = {}, released = {}}
    local objects = {}
    local env = setmetatable({}, {__index = _G})
    env._G = env
    env.db = {actor = nil, storage = {}, zone_by_name = {}, artefacts = {}}
    env.get_console = function()
        return {execute = function(_, line) calls.console[#calls.console + 1] = line end}
    end
    env.log = function(text) calls.logged[#calls.logged + 1] = text end
    env.time_global = function() return 1000 end
    env.alife = function()
        return {
            create = function(_, section, pos, lvid, gvid)
                calls.created[#calls.created + 1] = {section = section, position = pos}
                return {id = 9000 + #calls.created, section_name = function() return section end}
            end,
            object = function(_, id) return objects[id] end,
            story_object = function(_, story) return calls.story and calls.story[story] end,
            release = function(_, object) calls.released[#calls.released + 1] = object end
        }
    end
    calls.map = {}
    env.level = {
        map_add_object_spot = function(id, kind, hint)
            calls.spots[#calls.spots + 1] = {id, kind, hint, ser = false}
            calls.map[id .. ":" .. kind] = (calls.map[id .. ":" .. kind] or 0) + 1
        end,
        map_add_object_spot_ser = function(id, kind, hint)
            calls.spots[#calls.spots + 1] = {id, kind, hint, ser = true}
            calls.map[id .. ":" .. kind] = (calls.map[id .. ":" .. kind] or 0) + 1
        end,
        map_remove_object_spot = function(id, kind)
            calls.spots[#calls.spots + 1] = {id, kind, false}
            calls.map[id .. ":" .. kind] = math.max((calls.map[id .. ":" .. kind] or 1) - 1, 0)
        end,
        map_has_object_spot = function(id, kind) return calls.map[id .. ":" .. kind] or 0 end,
        name = function() return "l01_escape" end,
        object_by_id = function(id) return objects[id] end
    }
    env.game = {translate_string = function(id) return id end}
    env.has_alife_info = function(name) return calls.infos[name] == true end
    env.xr_logic = {pstor_retrieve = function(_, key, default) return calls.infos[key] or default end,
        pstor_store = function(_, key, value) calls.infos[key] = value end}
    env.task = {completed = "completed", fail = "fail", in_progress = "in_progress"}
    calls.task_states, calls.tasks = {}, {}
    env.level_tasks = {set_task_state = function(state, id, objective)
        calls.task_states[#calls.task_states + 1] = {state = state, id = id, objective = objective}
    end}
    env.delete = {delet_escape_volka = function() calls.volk = (calls.volk or 0) + 1 return "released" end}
    -- An actor that holds portions and tasks the way the engine's does.
    calls.actor = {
        give_info_portion = function(_, name) calls.infos[name] = true end,
        disable_info_portion = function(_, name) calls.infos[name] = nil end,
        get_task_state = function(_, id) return calls.tasks[id] end
    }
    local chunk = assert(loadfile(source_path))
    setfenv(chunk, env)
    chunk()
    return env, calls, objects
end

do
    local env, calls = fixture()
    -- install() must survive an environment where none of the mod's modules exist: every repair is wrapped, so a
    -- missing module costs that one repair and nothing else. This is the case on a save loaded before the module
    -- the repair needs has been compiled, and it is what keeps the pack from ever being the reason a game dies.
    env.install()
    check(true, "install() completes with no mod modules present")
    equal(#calls.console, 0, "installing writes nothing to the console")

    local before = #calls.created
    env.install()
    equal(#calls.created, before, "installing twice creates nothing twice")
end

-- Map markers. The mod places every one of its own markers the volatile way, from a scan that may run before
-- the object exists; the pack turns each call into a request that is honoured with the spot the save keeps.
do
    local env, calls, objects = fixture()
    env.clock_ms = 1000
    env.time_global = function() return env.clock_ms end
    local updates = 0
    env.bind_stalker = {actor_binder = {
        update = function() updates = updates + 1 end,
        net_spawn = function() return true end,
        net_destroy = function() end
    }}
    env.db.actor = {}
    local old_calls = 0
    env.skladu = {metka_na_glyshilky_mnlt = function() old_calls = old_calls + 1 end}
    env.escape_tasks = {process_info_portion = function(info) calls.handled = info end}
    env.install()
    local binder = env.bind_stalker.actor_binder
    local function tick(ms)
        env.clock_ms = env.clock_ms + ms
        binder.update({}, 1)
    end

    -- The jammer is asked for before it exists, and created a few actions later in the same frame: the mod's
    -- own function would have scanned, found nothing and said nothing. Now the request waits for the object.
    env.skladu.metka_na_glyshilky_mnlt()
    equal(old_calls, 0, "the mod's volatile marker function is replaced, not called")
    objects[700] = {id = 700, section_name = function() return "mil_glyshilka_monolita" end, name = function() return "x" end}
    tick(200)
    equal(#calls.spots, 1, "the spot appears on the first update after the object does")
    equal(calls.spots[1][2], "green_location", "of the mod's own kind")
    equal(calls.spots[1][3], "mtka_na_glyshky_mnlt", "with the mod's own caption")
    equal(calls.spots[1].ser, true, "and it is written into the save")
    tick(200)
    tick(200)
    equal(#calls.spots, 1, "the same spot is never placed twice")

    -- The portion that ends the errand takes the marker down.
    calls.infos.mil_monolita_glyshilke_smerti = true
    tick(200)
    equal(#calls.spots, 2, "the end portion removes it")
    equal(calls.spots[2][3], false, "as a removal")
    equal(calls.map["700:green_location"], 0, "and the map is clean")

    -- A save from before the pack: the start portion alone is enough, no call needed.
    calls.infos.mil_monolita_glyshilke_smerti = nil
    calls.infos["mil_jora2_pred ydarom"] = true
    binder.net_spawn({})
    tick(200)
    equal(#calls.spots, 3, "a held start portion places the marker on its own")
    equal(calls.spots[3].ser, true, "serialised like the rest")

    -- An object that only appears after the sweep has passed its id is still found: the sweep wraps, rests
    -- and comes round again, and a rule in wait costs one slice of the simulation per tick, never a full pass.
    -- The engine drops an object's spots with the object; the fixture has to do the same by hand.
    objects[700] = nil
    calls.map["700:green_location"] = nil
    binder.net_spawn({})
    tick(200)
    objects[700] = {id = 700, section_name = function() return "mil_glyshilka_monolita" end, name = function() return "x" end}
    local before, ticks = #calls.spots, 0
    while #calls.spots == before and ticks < 120 do
        tick(200)
        ticks = ticks + 1
    end
    equal(#calls.spots, before + 1, "a late object is found by the next pass")
    check(ticks <= 60, "within one sweep and one rest: " .. ticks .. " ticks")

    -- The two markers the mod placed from its info handlers, on a story object it never checked for.
    calls.story = {[93] = {id = 93, section_name = function() return "space_restrictor" end}}
    env.escape_tasks.process_info_portion("esc_post_pianka_tolik")
    equal(calls.handled, nil, "the handler's own volatile placement is not run")
    tick(200)
    equal(calls.spots[#calls.spots][1], 93, "the story object gets its spot")
    equal(calls.spots[#calls.spots][3], "metka_dejyr_za_volka_text", "with the caption the handler used")
    env.escape_tasks.process_info_portion("something_else")
    equal(calls.handled, "something_else", "every other portion still reaches the mod's handler")
end

-- The settles that run from the actor's update: an actor, a binder, and a tick.
local function settled()
    local env, calls, objects = fixture()
    local binder = {net_spawn = function() return true end, update = function() calls.updates = (calls.updates or 0) + 1 end,
        net_destroy = function() end}
    env.bind_stalker = {actor_binder = binder}
    env.db.actor = calls.actor
    env.install()
    return env, calls, objects, function() env.bind_stalker.actor_binder.update({}, 1) end
end

-- Nemo, the immortal the pack put into the nationalist camp in 1.0.8: withdrawn, once, from the saves that have him.
do
    local env, calls, objects, tick = settled()
    objects[10] = {id = 10, section_name = function() return "agro_nemo_tyt_vujivet" end}
    objects[11] = {id = 11, section_name = function() return "agro_nac_prosto_statist" end}
    tick()
    equal(#calls.released, 0, "nothing happens before the quest is under way")
    calls.infos.agro_karabin_strt = true
    tick()
    equal(#calls.released, 1, "the immortal goes")
    equal(calls.released[1], objects[10], "and only he")
    equal(calls.infos.ild_agro_nemo_withdrawn, 1, "remembered in the save")
    objects[12] = {id = 12, section_name = function() return "agro_nemo_tyt_vujivet" end}
    tick()
    equal(#calls.released, 1, "never twice")
    equal(calls.updates, 3, "and the mod's own update ran every time")
end

-- The Toymaker's peaceful branch settles the poltergeist story; the shoot-him saves of 1.0.8/1.0.9 get their double.
do
    local env, calls, objects, tick = settled()
    tick()
    equal(calls.infos.agro_polter_sdox, nil, "nothing before the talk")
    calls.infos.agro_ydivitilnui_cirk = true
    tick()
    equal(calls.infos.agro_polter_sdox, true, "the peaceful outcome counts as the story settled")
    local fight, fight_calls, _, fight_tick = settled()
    fight_calls.infos.agro_ydivitilnui_cirk, fight_calls.infos.agro_polter_spawn = true, true
    fight_tick()
    equal(fight_calls.infos.agro_polter_sdox, nil, "a fight under way is left to run")

    local alive, alive_calls, alive_objects, alive_tick = settled()
    alive_objects[20] = {id = 20, section_name = function() return "agro_kotelnui_ybiica" end, alive = function() return true end}
    alive_calls.infos.agro_igryshechnik_start, alive_calls.infos.ara_tak_vtorogo_xyilu_ne_nado = true, true
    alive_tick()
    equal(alive_calls.infos.ara_tak_vtorogo_xyilu_ne_nado, nil, "a living killer of such a save gets the portion taken back")
    equal(alive_calls.infos.aro_gg_v_glaz_popal, nil, "and keeps his talk")
    local dead, dead_calls, dead_objects, dead_tick = settled()
    dead_objects[20] = {id = 20, section_name = function() return "agro_kotelnui_ybiica" end, alive = function() return false end}
    dead_calls.infos.agro_igryshechnik_start, dead_calls.infos.ara_tak_vtorogo_xyilu_ne_nado = true, true
    dead_tick()
    equal(dead_calls.infos.ara_tak_vtorogo_xyilu_ne_nado, nil, "a dead one loses the portion")
    equal(dead_calls.infos.aro_gg_v_glaz_popal, true, "and gets his double at the stairs")
    local gone, gone_calls, _, gone_tick = settled()
    gone_calls.infos.agro_igryshechnik_start, gone_calls.infos.ara_tak_vtorogo_xyilu_ne_nado = true, true
    gone_tick()
    equal(gone_calls.infos.aro_gg_v_glaz_popal, true, "as does one the corpse cleaner took")
    local talked, talked_calls, _, talked_tick = settled()
    talked_calls.infos.agro_igryshechnik_start, talked_calls.infos.ara_tak_vtorogo_xyilu_ne_nado = true, true
    talked_calls.infos.agro_ybiica_start = true
    talked_tick()
    equal(talked_calls.infos.ara_tak_vtorogo_xyilu_ne_nado, true, "a save that had the talk is the ordinary case and stays")
end

-- Volk's two optional jobs close when the story removes him, or on a save where it already has.
do
    local env, calls, _, tick = settled()
    calls.tasks.esc_poisk_novichkov_kvest, calls.tasks.tolik_i_volk_kvest = "in_progress", "in_progress"
    equal(env.delete.delet_escape_volka(), "released", "the mod's own removal runs")
    equal(calls.volk, 1, "once")
    equal(#calls.task_states, 2, "and both open jobs are closed with it")
    equal(calls.task_states[1].id, "esc_poisk_novichkov_kvest", "the search")
    equal(calls.task_states[1].state, "fail", "failed, which fails its open steps")
    equal(calls.task_states[2].id, "tolik_i_volk_kvest", "and the duty")
    equal(calls.task_states[2].state, "fail", "failed, since it was never served")
    local served, served_calls, _, served_tick = settled()
    served_calls.tasks.tolik_i_volk_kvest = "in_progress"
    served_calls.tasks.esc_poisk_novichkov_kvest = "completed"
    served_calls.infos.esc_repka_konec_nax = true
    served_tick()
    equal(#served_calls.task_states, 0, "while Volk is there, nothing")
    served_calls.infos.brr_obersh_posli_stepu = true
    served_tick()
    equal(#served_calls.task_states, 1, "once the story has moved on, the job still open is closed")
    equal(served_calls.task_states[1].state, "completed", "as completed, since the duty was served")
    served_tick()
    equal(#served_calls.task_states, 1, "once")
end

-- Yura back from offline resumes the stage his portions name instead of walking off to the tunnel.
do
    local env, calls = fixture()
    local activated, originals = {}, 0
    env.xr_logic.initialize_obj = function() originals = originals + 1 end
    env.xr_logic.configure_schemes = function(object, ini) return ini end
    env.xr_logic.activate_by_section = function(object, ini, section, loading) activated[#activated + 1] = section end
    env.install()
    local ini = {section_exist = function(_, section) return section ~= "nowhere" end}
    local yura = {section = function() return "esc_qra2" end, name = function() return "esc_qra2" end,
        spawn_ini = function() return ini end}
    env.xr_logic.initialize_obj(yura, {}, false, nil, "stalker")
    equal(originals, 1, "a fresh Yura with no portions takes the mod's own path")
    calls.infos.esc_qra2_start_v_tonnele = true
    env.xr_logic.initialize_obj(yura, {}, false, nil, "stalker")
    equal(activated[1], "remark1", "the tunnel stage resumes at its wait")
    calls.infos.esc_qra_pered_tainikom1 = true
    env.xr_logic.initialize_obj(yura, {}, false, nil, "stalker")
    equal(activated[2], "walker45", "the stash stage at the stash")
    calls.infos.esc_qra_ia_nashel_xabar_t2 = true
    env.xr_logic.initialize_obj(yura, {}, false, nil, "stalker")
    equal(activated[3], "walker7", "the latest stage wins")
    env.xr_logic.initialize_obj(yura, {}, true, nil, "stalker")
    equal(originals, 2, "a Yura loaded from a save keeps the section the save holds")
    local other = {section = function() return "esc_dark_kopatel" end, name = function() return "digger" end,
        spawn_ini = function() return ini end}
    env.xr_logic.initialize_obj(other, {}, false, nil, "stalker")
    equal(originals, 3, "and any other NPC is the mod's own business")
    equal(#activated, 3, "with nothing resumed for him")
end

-- The spy's stash marker comes down once the box has been emptied, and stays down.
do
    local env, calls, objects = fixture()
    env.clock_ms = 1000
    env.time_global = function() return env.clock_ms end
    env.bind_stalker = {actor_binder = {update = function() end, net_spawn = function() return true end,
        net_destroy = function() end}}
    env.db.actor = calls.actor
    env.install()
    local function tick(ms)
        env.clock_ms = env.clock_ms + ms
        env.bind_stalker.actor_binder.update({}, 1)
    end
    local empty = false
    objects[800] = {id = 800, section_name = function() return "esc_tainik_mamkinogo_shpiona" end,
        name = function() return "stash" end, is_inv_box_empty = function() return empty end}
    calls.infos.esc_pervui_shpion_pacanchika_poshadil = true
    tick(200)
    equal(calls.map["800:crlc_big"], 1, "the spared spy's stash carries its spot")
    empty = true
    tick(200)
    equal(calls.map["800:crlc_big"], 0, "and loses it once the box is empty")
    equal(calls.infos.ild_marker_spy_stash_done, 1, "remembered in the save")
    empty = false
    tick(200)
    tick(200)
    equal(calls.map["800:crlc_big"], 0, "never to come back")
end

print("PASS " .. tests .. " checks")
