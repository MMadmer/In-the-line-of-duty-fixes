local source_path = arg[1] or "payload/gamedata/scripts/ild_script_repairs.script"
local tests = 0

local function check(value, message)
    assert(value, message)
    tests = tests + 1
end

local function equal(actual, expected, message)
    check(actual == expected, message .. ": expected " .. tostring(expected) .. ", got " .. tostring(actual))
end

local function fixture()
    local calls = {tutorials = {}, events = {}, spots = {}, food = {}, spawned = {}, removed = {}, detector = {},
        released = {}, init_btn = {}, on_info = {}, menu_toggles = {}, object_queries = 0}
    local objects = {}
    local clock = 100
    local env = setmetatable({}, {__index = _G})
    env._G = env
    env.db = {artefacts = {}}
    env.game = {
        start_tutorial = function(name) calls.tutorials[#calls.tutorials + 1] = name end,
        get_game_time = function()
            return {value = clock, diffSec = function(self, old) return self.value - old.value end}
        end
    }
    env.alife = function()
        return {
            object = function(_, id)
                calls.object_queries = calls.object_queries + 1
                return objects[id]
            end,
            release = function(_, object, online)
                calls.released[#calls.released + 1] = {object = object, online = online}
                objects[object.id] = nil
            end
        }
    end
    env.level = {
        map_remove_object_spot = function(id, kind) calls.spots[#calls.spots + 1] = {id, kind} end,
        main_input_receiver = function() return calls.receiver end,
        start_stop_menu = function(window, flag) calls.menu_toggles[#calls.menu_toggles + 1] = {window, flag} end
    }
    env.ui_events = {WINDOW_LBUTTON_DB_CLICK = 9}
    env.new_life = {}
    env.ild_recipe_repairs = {install = function() end}
    env.moa_agro = {new_gg_2 = function(value) return value end}
    env.delete = {esc_ydalaem_bbbbbbbbbbbbbtttttttttttrrrrrrrrrrrrr = function(value) return value end}
    env.ui_main_menu = {main_menu = {
        OnButton_credits_clicked = function(self, value)
            assert(env.db.actor)
            calls.credits = (calls.credits or 0) + 1
            return value
        end
    }}
    env.sound_theme = {ph_snd_themes = {unchanged = {"sentinel"}}}
    env.bind_monster = {generic_object_binder = {
        death_callback = function(self, victim, who)
            assert(who and env.db.actor)
            calls.original_death = (calls.original_death or 0) + 1
            return "original"
        end
    }}
    env.xr_logic = {
        parse_condlist = function(npc, section, field, source)
            return {npc = npc, section = section, field = field, source = source}
        end,
        issue_event = function(object, storage, name, victim, who)
            calls.events[#calls.events + 1] = {object = object, storage = storage, victim = victim, who = who}
        end
    }
    env.smart_terrain = {on_death = function(id) calls.terrain = id end}
    env.hit = setmetatable({fire_wound = 5}, {
        __call = function() return {bone = function(self, name) self.bone_name = name end} end
    })
    env.se_zones = {se_zone_anom = {}, se_zone_visual = {}}
    env.cse_anomalous_zone = {update = function() calls.native_anom = (calls.native_anom or 0) + 1 end}
    env.cse_zone_visual = {update = function() calls.native_visual = (calls.native_visual or 0) + 1 end}
    env.main_sleep = {eat_food = function() error("unpatched food function") end}
    env.amk = {remove_items = function() error("unpatched item removal") end}
    env.hidden_slots = {
        BkgrWnd = {
            InitControls = function(self)
                -- Mirrors the mod: assign self.btn, call init_btn(k), then register ClickBtn[k].
                self.registered = {}
                for index = 1, 3 do
                    self.btn = self.buttons[index]
                    env.hidden_slots.init_btn(index)
                    self.registered["check_button_" .. index] = self.ClickBtn[index]
                end
                return "initialised"
            end
        },
        init_btn = function(number, section) calls.init_btn[#calls.init_btn + 1] = {number, section} end,
        spawn_item_in_inv = function(section) calls.spawned[#calls.spawned + 1] = section end,
        remove_item = function(id) calls.removed[#calls.removed + 1] = id end,
        on_info = function(info_id) calls.on_info[#calls.on_info + 1] = info_id end,
        inventory_close = function() error("unpatched inventory_close") end
    }
    env.bind_det_arts = {
        start_update = function()
            calls.detector_starts = (calls.detector_starts or 0) + 1
            env.db.actor:set_fastcall(env.bind_det_arts.update, env.db.actor)
        end,
        update = function() error("unpatched detector function") end,
        det_indy = function() calls.detector[#calls.detector + 1] = "indy" end,
        det_normal = function() calls.detector[#calls.detector + 1] = "normal" end,
        det_super = function() calls.detector[#calls.detector + 1] = "super" end,
        iteration_del_spot = function() calls.detector[#calls.detector + 1] = "clear" end
    }
    env.bind_stalker = {actor_binder = {
        load = function(self, storage)
            self.st = storage
            return "loaded"
        end,
        net_spawn = function(self, success)
            if success == false then return false end
            env.db.actor = self.object
            return true
        end,
        update = function(self, value)
            calls.actor_update = value
            if self.first_update then
                self.first_update = false
                env.bind_det_arts.start_update()
            end
        end,
        net_destroy = function() calls.destroyed = (calls.destroyed or 0) + 1 return "destroyed" end
    }}
    local module = setmetatable({}, {__index = env})
    local chunk = assert(loadfile(source_path))
    setfenv(chunk, module)
    chunk()
    local function actor()
        return {
            items = {}, slots = {},
            object = function(self, section) return self.items[section] end,
            object_count = function(self) return self.inventory_size or 0 end,
            item_in_slot = function(self, slot) return self.slots[slot] end,
            eat = function(self, item)
                assert(item, "eat must never receive nil")
                calls.food[#calls.food + 1] = item
            end,
            set_fastcall = function(self, callback) self.fastcall = callback end,
            position = function() return {sub = function() return "direction" end} end
        }
    end
    return env, calls, objects, module, actor
end

local function make_item(section, id)
    return {section = function() return section end, id = function() return id end}
end

do
    local env, _, _, module = fixture()
    module.install()
    local npc = {name = function() return "labx_psevdogiant" end,
        section = function() return "gigant_strong" end}
    local broken = "{=actor_in_zone(x18_dlia_sevducha_gg_tyt5)} %=bar_arena_hit% mob_walker@6"
    local fixed = "{=actor_in_zone(x18_dlia_sevducha_gg_tyt5)} %=bar_arena_hit% mob_walker@5"
    for index = 1, 4 do
        local section = "mob_walker@" .. index
        local parsed = env.xr_logic.parse_condlist(npc, section, "on_info", broken)
        equal(parsed.source, fixed, "exact X18 transition selects the existing state")
        equal(parsed.npc, npc, "condition parsing retains its original object")
        equal(parsed.section, section, "condition parsing retains its original section")
    end
    equal(env.xr_logic.parse_condlist(npc, "mob_walker@5", "on_info", broken).source, broken,
        "other giant sections are not changed")
    equal(env.xr_logic.parse_condlist(npc, "mob_walker@1", "on_actor_dist_le_nvis", broken).source, broken,
        "distance damage triggers are not changed")
    equal(env.xr_logic.parse_condlist(npc, "mob_walker@1", "on_info", "x" .. broken).source, "x" .. broken,
        "nonmatching condition fingerprints are not changed")
    equal(env.xr_logic.parse_condlist(npc, "mob_walker@1", "on_info", " " .. broken).source, " " .. fixed,
        "insignificant whitespace is preserved")
    local other = {name = function() return "other_giant" end, section = npc.section}
    equal(env.xr_logic.parse_condlist(other, "mob_walker@1", "on_info", broken).source, broken,
        "other giants are not changed")
    other = {name = npc.name, section = function() return "other_section" end}
    equal(env.xr_logic.parse_condlist(other, "mob_walker@1", "on_info", broken).source, broken,
        "other object sections are not changed")
    equal(env.xr_logic.parse_condlist(nil, "mob_walker@1", "on_info", broken).source, broken,
        "nil-object parsing is forwarded unchanged")
end

do
    local env, calls, _, module, actor = fixture()
    module.install()
    equal(env.new_life.new_gg_2("agroprom"), "agroprom", "missing story callback aliases the existing implementation")
    equal(env.delete.esc_ydalaem_bbbbbbbbbbbbbtttttttttttrrrrr("btr"), "btr",
        "missing BTR callback aliases the existing implementation")
    env.ui_main_menu.main_menu.OnButton_credits_clicked({})
    equal(calls.tutorials[1], "credits_seq", "main menu without actor opens credits")
    env.db.actor = actor()
    equal(env.ui_main_menu.main_menu.OnButton_credits_clicked({}, "mod result"), "mod result",
        "loaded actor retains the mod callback and its return")
    equal(calls.credits, 1, "loaded actor callback called once")
    local before = env.bind_stalker.actor_binder.net_spawn
    module.install()
    equal(env.bind_stalker.actor_binder.net_spawn, before, "install is idempotent")
    equal(env.sound_theme.ph_snd_themes.moa_psi_antena[1], [[characters_voice\scenario\yantar\psy_voices_1]],
        "psi sound retains path separators")
    equal(env.sound_theme.ph_snd_themes.grey_grey_later[1], [[characters_voice\scenario\garbage\grey_grey_later]],
        "grey sound retains path separators")
    equal(env.sound_theme.ph_snd_themes.unchanged[1], "sentinel", "unrelated sound entries remain unchanged")
end

do
    local env, _, _, module = fixture()
    local existing = function() return "addon" end
    env.new_life.new_gg_2 = existing
    env.delete.esc_ydalaem_bbbbbbbbbbbbbtttttttttttrrrrr = existing
    module.install()
    equal(env.new_life.new_gg_2, existing, "existing story callback is not replaced by an alias")
    equal(env.delete.esc_ydalaem_bbbbbbbbbbbbbtttttttttttrrrrr, existing,
        "existing BTR callback is not replaced by an alias")
end

do
    local env, calls, _, module, actor = fixture()
    local original_spawn = env.bind_stalker.actor_binder.net_spawn
    module.install()
    local registry = env.db.artefacts
    equal(env.bind_stalker.actor_binder.net_spawn({object = actor()}, false), false,
        "failed actor spawn is returned unchanged")
    equal(env.db.artefacts, registry, "failed spawn does not discard detector state")
    local binder = {object = actor(), first_update = true}
    original_spawn(binder)
    -- Installation can happen inside the original spawn call before its wrapper is entered.
    env.bind_stalker.actor_binder.update(binder, 21)
    equal(binder.object.fastcall(), false, "actor-first bootstrap replaces the already registered callback")
    equal(calls.detector[#calls.detector], "clear", "actor-first bootstrap installs the detector implementation")
    local starts = calls.detector_starts
    env.bind_stalker.actor_binder.update(binder, 22)
    equal(calls.detector_starts, starts, "actor-first bootstrap settles after the initial update")
end

do
    local env, calls, _, module, actor = fixture()
    module.install()
    local player = actor()
    local binder = {object = player, first_update = true}
    env.db.artefacts[81] = {spot = true, tim_beep = 123}
    equal(env.bind_stalker.actor_binder.net_spawn(binder), true, "actor spawn result is retained")
    equal(calls.spots[1][1], 81, "old detector spots are removed at actor replacement")
    equal(next(env.db.artefacts), nil, "detector IDs are not reused across actor sessions")
    env.bind_stalker.actor_binder.update(binder, 12)
    equal(calls.actor_update, 12, "actor update arguments are retained")
    equal(calls.detector_starts, 1, "original first update does not cause a duplicate detector scan")
    env.bind_stalker.actor_binder.update(binder, 13)
    equal(calls.detector_starts, 1, "detector callback is not registered every frame")
    equal(player.fastcall(), false, "empty detector slot keeps callback alive")
    equal(calls.detector[#calls.detector], "clear", "empty slot clears detector markers")
    local function dispatch(section)
        player.slots[1] = section and make_item(section, 1) or nil
        equal(player.fastcall(), false, "active detector callback returns false")
        return calls.detector[#calls.detector], #calls.detector
    end
    local last, count = dispatch("det_artefact_pro")
    equal(last, "normal", "pro detector dispatch")
    equal(dispatch("wpn_pm"), "clear", "spots are swept once the detector is put away")
    local _, again = dispatch("wpn_pm")
    equal(again, count + 1, "no sweep runs while nothing can have added spots")
    equal(dispatch("det_artefact_indy"), "indy", "indy detector dispatch")
    _, again = dispatch("wpn_pm")
    equal(again, count + 2, "the indy detector adds no spots, so nothing is swept")
    equal(dispatch("det_artefact_super"), "super", "super detector dispatch")
    equal(dispatch(nil), "clear", "an emptied slot sweeps the super detector spots")
    env.db.actor = nil
    equal(player.fastcall(), true, "absent actor retires detector callback")
    local replacement = {object = actor()}
    env.bind_stalker.actor_binder.net_spawn(replacement)
    env.bind_stalker.actor_binder.update(replacement, 14)
    equal(calls.detector_starts, 2, "new actor gets a callback when the mod's local arts flag is already true")

    player = env.db.actor
    env.main_sleep.eat_food()
    equal(#calls.food, 0, "empty food inventory causes no native eat calls")
    player.items.bread, player.items.kolbasa = "bread", "kolbasa"
    env.main_sleep.eat_food()
    equal(calls.food[1], "bread", "first available food is selected")
    equal(#calls.food, 1, "one sleep food action consumes only one item")
    player.items.conserva = "conserva"
    env.main_sleep.eat_food()
    equal(calls.food[2], "conserva", "original food priority is retained")
    player.items = {kolbasa = "kolbasa"}
    env.main_sleep.eat_food()
    equal(calls.food[3], "kolbasa", "last food choice is supported")
    env.db.actor = nil
    env.main_sleep.eat_food()
    equal(#calls.food, 3, "food helper is safe without an actor")
end

do
    local env, calls, objects, module, actor = fixture()
    module.install()
    local player = actor()
    env.bind_stalker.actor_binder.net_spawn({object = player})
    local function button() return {Show = function(self, value) self.shown = value end} end
    local untouched = function() return "other slot" end
    check(env.hidden_slots.BkgrWnd.AddCallback == nil, "inherited C++ methods are never taken from the class")
    for section, expected in pairs({wpn_knife = "fake_wpn_knife", wpn_knife_6x4 = "fake_wpn_knife_6x4",
        wpn_knife_6x2 = "fake_wpn_knife_6x2", lom = "fake_lom", wpn_knife_nkvd = "fake_wpn_knife_nkvd"}) do
        local window = {buttons = {button(), button(), button()}, ClickBtn = {[2] = untouched, [3] = untouched}}
        player.slots[0] = make_item(section, 712)
        objects[712] = player.slots[0]
        equal(env.hidden_slots.BkgrWnd.InitControls(window), "initialised", "original InitControls result is retained")
        equal(window.ild_knife_button, window.buttons[1], "the knife button is captured while InitControls runs")
        equal(window.registered.check_button_2, untouched, "other hidden-slot callbacks are preserved")
        window.registered.check_button_1()
        equal(calls.spawned[#calls.spawned], expected, "currently slotted knife is converted")
        equal(calls.removed[#calls.removed], 712, "the same knife is removed")
        equal(window.buttons[1].shown, false, "the matching knife button is hidden")
        check(window.buttons[2].shown == nil and window.buttons[3].shown == nil, "other buttons keep their state")
    end
    equal(#calls.init_btn, 15, "init_btn still runs for every button")
    local window = {buttons = {button(), button(), button()}, ClickBtn = {}}
    env.hidden_slots.BkgrWnd.InitControls(window)
    local spawned, removed = #calls.spawned, #calls.removed
    player.slots[0] = make_item("unknown_addon_knife", 713)
    objects[713] = player.slots[0]
    window.registered.check_button_1()
    equal(#calls.spawned, spawned, "unknown knife sections are not transformed")
    player.slots[0] = make_item("wpn_knife", 714)
    window.registered.check_button_1()
    equal(#calls.spawned, spawned, "a knife without a server object is not converted again")
    player.slots[0] = nil
    window.registered.check_button_1()
    equal(#calls.removed, removed, "empty slot is not removed")
    check(window.buttons[1].shown == nil, "the button stays visible when nothing was converted")
    env.hidden_slots.init_btn(1, "wpn_knife")
    equal(calls.init_btn[#calls.init_btn][2], "wpn_knife", "init_btn outside InitControls passes through")
end

do
    local env, calls, objects, module = fixture()
    module.install()
    local existing = {spot = true, tim_beep = 25}
    env.db.artefacts[20] = existing
    objects[20] = {section_name = function() return "af_medusa" end}
    objects[21] = {section_name = function() return "wpn_pm" end}
    objects[22] = {section_name = function() return "af_dummy_spring" end}
    local zone = {last_spawn_time = {value = 1}, artefact_spawn_idle = 20, artefact_spawn_rnd = 100,
        spawn_artefacts = function() calls.zone_spawn = (calls.zone_spawn or 0) + 1 end}
    env.se_zones.se_zone_anom.update(zone)
    equal(calls.native_anom, 1, "native anomaly update remains first in the path")
    equal(calls.zone_spawn, 1, "void native spawn still executes")
    equal(calls.object_queries, 0, "zone update only marks the registry dirty")
    local visual = {last_spawn_time = {value = 1}, artefact_spawn_idle = 20, artefact_spawn_rnd = 100,
        spawn_artefacts = zone.spawn_artefacts}
    env.se_zones.se_zone_visual.update(visual)
    equal(calls.object_queries, 0, "multiple zones do not scan independently")
    env.db.actor = {set_fastcall = function() end}
    env.bind_stalker.actor_binder.update({}, 1)
    equal(calls.object_queries, 65534, "one actor update reconciles the entire respawn batch")
    equal(env.db.artefacts[20], existing, "existing detector state survives reconciliation")
    equal(env.db.artefacts[21], nil, "non-artifact server objects are not registered")
    equal(env.db.artefacts[22].spot, false, "new actual artifact starts without a marker")
    equal(env.db.artefacts[22].tim_beep, 0, "new actual artifact has a valid beep timestamp")
    env.bind_stalker.actor_binder.update({}, 2)
    equal(calls.object_queries, 65534, "no reconciliation scan runs on an unchanged frame")
    env.se_zones.se_zone_anom.update(zone)
    equal(calls.zone_spawn, 2, "respawn interval is not shortened")
    zone.last_spawn_time = {value = 1}
    env.se_zones.se_zone_visual.update(zone)
    equal(calls.native_visual, 2, "visual zone uses its matching native base")
    equal(calls.zone_spawn, 3, "visual zone also supports void spawning")
    zone.last_spawn_time = {value = 1}
    zone.artefact_spawn_rnd = 0
    env.se_zones.se_zone_visual.update(zone)
    equal(calls.zone_spawn, 3, "spawn probability is retained")
end

do
    local env, calls, objects, module, actor = fixture()
    module.install()
    local player = actor()
    env.bind_stalker.actor_binder.net_spawn({object = player})
    player.items[0], player.inventory_size = make_item("part", 200), 1
    objects[200] = {id = 200}
    env.amk.remove_items("part", 1)
    equal(#calls.released, 1, "inventory index zero is consumed")
    equal(calls.released[1].object.id, 200, "the exact server object is released")
    equal(calls.released[1].online, true, "original online release semantics are retained")

    player.items[0], player.items[1], player.items[2], player.items[3], player.inventory_size =
        make_item("part", 300), make_item("other", 301), make_item("part", 302), make_item("part", 303), 4
    objects[300], objects[301], objects[302], objects[303] = {id = 300}, {id = 301}, nil, {id = 303}
    env.amk.remove_items("part", 2)
    equal(#calls.released, 3, "missing server objects do not count toward the requested amount")
    equal(calls.released[2].object.id, 303, "removal still proceeds in descending inventory order")
    equal(calls.released[3].object.id, 300, "index zero supplies the remaining requested item")
    check(objects[301], "unrelated inventory sections survive")
    objects[300], objects[303] = {id = 300}, {id = 303}
    env.amk.remove_items("part", 0)
    equal(#calls.released, 3, "zero requested items changes nothing")
    env.amk.remove_items("part")
    equal(#calls.released, 4, "omitted count still defaults to one")
    check(objects[300], "removal stops at the requested amount")
end

do
    local env, _, _, module = fixture()
    module.install()
    local binder = {}
    equal(env.bind_stalker.actor_binder.load(binder, {disable_input_time = {}}), "loaded",
        "actor load result is retained")
    equal(binder.st.disable_input_idle, 30, "restored timestamp gains the missing punch timeout")
    env.bind_stalker.actor_binder.load(binder, {disable_input_time = {}, disable_input_idle = 8})
    equal(binder.st.disable_input_idle, 8, "an existing custom timeout is preserved")
    env.bind_stalker.actor_binder.load(binder, {disable_input_time = {}, disable_input_idle = 0})
    equal(binder.st.disable_input_idle, 0, "zero timeout is not mistaken for an absent value")
    env.bind_stalker.actor_binder.load(binder, {})
    equal(binder.st.disable_input_idle, nil, "unrelated saves do not gain input state")
    binder.st = {disable_input_time = {}}
    env.bind_stalker.actor_binder.update(binder, 1)
    equal(binder.st.disable_input_idle, 30, "actor-first bootstrap repairs a previously restored timestamp")
end

do
    local env, calls, _, module, actor = fixture()
    module.install()
    local victim = {id = function() return 47 end, position = function() return {} end,
        hit = function(self, value) calls.impulse = value end}
    local binder = {object = victim, st = {mob_death = {}, active_section = "active", active_scheme = "scheme",
        scheme = {}}}
    env.db.actor = actor()
    env.bind_monster.generic_object_binder.death_callback(binder, victim, nil)
    equal(calls.terrain, 47, "nil-killer deaths still notify smart terrain")
    equal(#calls.events, 2, "nil-killer deaths keep both scheme events")
    equal(calls.events[1].who, nil, "killer is not replaced with a fake object")
    equal(calls.impulse.power, 1, "original corpse impulse remains")
    equal(calls.impulse.bone_name, "pelvis", "original impulse bone remains")
    equal(env.bind_monster.generic_object_binder.death_callback(binder, victim, {}), "original",
        "ordinary deaths retain the original callback")
    equal(calls.original_death, 1, "ordinary callback executes once")
    env.db.actor = nil
    env.bind_monster.generic_object_binder.death_callback(binder, victim, nil)
    equal(#calls.events, 4, "actor absence still preserves death scheme events")
end

do
    local env, calls, _, module = fixture()
    module.install()
    local balance = 2999
    env.db.actor = {money = function() return balance end}
    env.dialogs = {relocate_money = function(npc, amount, direction)
        calls.money = {npc = npc, amount = amount, direction = direction}
    end}
    equal(env.new_life.dengi_esti(), false, "cache price rejects insufficient money")
    env.new_life.give_baaablo(nil, "seller")
    equal(calls.money, nil, "insufficient money is not removed")
    balance = 3000
    equal(env.new_life.dengi_esti(), true, "documented cache price is 3000")
    env.new_life.give_baaablo(nil, "seller")
    equal(calls.money.amount, 3000, "charge preserves documented price")
    equal(calls.money.direction, "out", "money leaves actor")
    local npc = {name = function() return "labx_psevdogiant" end, section = function() return "gigant_strong" end}
    local value = "{=actor_in_zone(x18_dlia_sevducha_gg_tyt5)}%=bar_arena_hit%mob_walker@6"
    equal(env.xr_logic.parse_condlist(npc, "mob_walker@1", "on_info", value).source,
        "{=actor_in_zone(x18_dlia_sevducha_gg_tyt5)}%=bar_arena_hit%mob_walker@5", "ini whitespace normalization is safe")
end

do
    local env, calls, _, module, actor = fixture()
    module.install()
    local player = actor()
    env.bind_stalker.actor_binder.net_spawn({object = player})
    local function button() return {Show = function(self, value) self.shown = value end} end
    local stat = {Show = function(self, value) self.shown = value end}
    local window = {buttons = {button(), button(), button()}, ClickBtn = {}, stat = stat}
    env.hidden_slots.on_info("ui_inventory")
    equal(#calls.on_info, 1, "the first open goes through the mod's on_info")
    env.hidden_slots.BkgrWnd.InitControls(window)
    local receiver = {attached = {}, shown = true,
        AttachChild = function(self, child) self.attached[#self.attached + 1] = child end,
        IsShown = function(self) return self.shown end}
    calls.receiver = receiver
    local before = #calls.init_btn
    env.hidden_slots.on_info("ui_inventory")
    equal(#calls.on_info, 1, "the second open reuses the overlay instead of rebuilding it")
    equal(window.owner, receiver, "the overlay follows the live inventory window")
    equal(receiver.attached[1], stat, "the detached container is attached again")
    equal(#calls.init_btn - before, 3, "button icons are refreshed for the current slots")
    equal(stat.shown, true, "the container is shown")
    env.hidden_slots.on_info("ui_inventory_hide")
    equal(calls.on_info[#calls.on_info], "ui_inventory_hide", "hide still goes through the mod's on_info")
    calls.receiver = nil
    env.hidden_slots.on_info("ui_inventory")
    equal(#calls.on_info, 3, "without a live receiver the mod's on_info runs")
    calls.receiver = receiver
    env.hidden_slots.inventory_close()
    equal(calls.menu_toggles[1][1], receiver, "inventory_close uses the live receiver")
    receiver.shown = false
    env.hidden_slots.inventory_close()
    equal(#calls.menu_toggles, 1, "a hidden receiver is not toggled")
    equal(env.bind_stalker.actor_binder.net_destroy({}), "destroyed", "net_destroy result is retained")
    env.hidden_slots.on_info("ui_inventory")
    equal(#calls.on_info, 4, "a level change drops the stale overlay")
end

print("script_repairs_tests: " .. tests .. " checks passed")
