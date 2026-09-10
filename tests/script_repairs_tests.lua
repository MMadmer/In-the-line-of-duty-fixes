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
        released = {}, init_btn = {}, on_info = {}, menu_toggles = {}, given = {}, taken = {}, news = {},
        amk_spawns = {}, treasure = {}, looks = {}, infos = {}, sounds = {}, statics = {}, tips = {},
        created = {}, task_states = {}, news = {}, played = {}, object_queries = 0}
    local objects = {}
    local clock = 100
    local env = setmetatable({}, {__index = _G})
    env._G = env
    env.db = {artefacts = {}, storage = {}}
    env.game = {
        start_tutorial = function(name) calls.tutorials[#calls.tutorials + 1] = name end,
        -- The engine hands back the id itself when a string table has no such entry.
        translate_string = function(id) return calls.strings and calls.strings[id] or id end,
        get_game_time = function()
            return {value = clock, diffSec = function(self, old) return self.value - old.value end}
        end
    }
    env.alife = function()
        return {
            create = function(_, section, position, lvid, gvid)
                calls.created[#calls.created + 1] = {section, position, lvid, gvid}
            end,
            object = function(_, id)
                calls.object_queries = calls.object_queries + 1
                return objects[id]
            end,
            story_object = function(_, story) return calls.story and calls.story[story] end,
            release = function(_, object, online)
                calls.released[#calls.released + 1] = {object = object, online = online}
                objects[object.id] = nil
            end
        }
    end
    env.clock_ms = 3600000
    env.time_global = function() return env.clock_ms end
    env.get_hud = function()
        return {
            GetCustomStatic = function(_, name) return calls.statics[name] end,
            AddCustomStatic = function(_, name)
                calls.statics[name] = {name = name}
                return calls.statics[name]
            end
        }
    end
    -- Stands in for the native bridge: "read setting_x" selects a key, get_string returns it.
    calls.settings = {radio_volume = "70"}
    env.get_console = function()
        return {
            execute = function(_, line)
                calls.console = calls.console or {}
                calls.console[#calls.console + 1] = line
                local key = string.match(line, "^ild_update read setting_(.+)$")
                if key then calls.selected = key end
            end,
            get_string = function() return calls.settings[calls.selected] or "" end
        }
    end
    env.ph_sound = {snd_source = {
        update = function(self)
            calls.sound_updates = (calls.sound_updates or 0) + 1
            return "updated"
        end
    }}
    env.xr_sound = {
        get_safe_sound_object = function(path)
            -- The engine fatals inside the constructor when the file is absent.
            if calls.absent_files[string.lower(path)] then error('Can\'t open wave file: ' .. path) end
            calls.sounds[#calls.sounds + 1] = path
            return {played = false, play_no_feedback = function(self) self.played = true end}
        end
    }
    calls.absent_files = {
        [ [[music\trava_y_doma_obrez]] ] = true,
        [ [[new\ost_mgnovenia_17]] ] = true,
        [ [[weapons\pm\pm_shoot]] ] = true,
        [ [[soundtrack\controller\7]] ] = true
    }
    env.xr_sleeper = {
        set_scheme = function(npc, ini, scheme, section, gulag_name)
            local storage = {}
            env.db.storage[npc:id()] = env.db.storage[npc:id()] or {}
            env.db.storage[npc:id()][scheme] = storage
            -- Gulag jobs prefix every path with the smart-terrain name.
            local raw = ini:r_string(section, 'path_main')
            storage.path_main = gulag_name ~= '' and (gulag_name .. '_' .. raw) or raw
            return 'sleeper set'
        end
    }
    env.death_manager = {drop_manager = {
        create_release_item = function(self)
            local known = {stalker = {}, dolg = {}}
            local drops = known[self.npc:character_community()]
            for _ in pairs(drops) do end
            calls.dropped = (calls.dropped or 0) + 1
            return 'dropped'
        end
    }}
    local function banner(name) return function() calls.statics[name] = {stale = true} end end
    env.new_life = {
        stalkerok_navuk_lvl1 = banner('chibi_rechi_navuk_lvl1'),
        chibi_dost_za_vse_kassetu = banner('chibi_dost_za_vse_kassetu'),
        stalkerok_mexanik_navuk_lvl1 = banner('chibi_mex_navuk_lvl1'),
        stalkerok_mexanik_navuk_lvl2 = banner('chibi_mex_navuk_lvl2'),
        stalkerok_navuk_poisk_lvl1 = banner('chibi_poisk_navuk_lvl1'),
        mne_nagrada_ot_rebu_za_shpionov = function(first, second)
            calls.rewards = (calls.rewards or 0) + 1
            return 'desert eagle'
        end
    }
    local function bench(floor)
        local class_table = {}
        for name, slot in pairs({rep_s1 = 1, rep_s2 = 2, rep_s6 = 6}) do
            class_table[name] = function()
                local item = env.db.actor:item_in_slot(slot)
                -- The level-2 bench's slot-6 handler carries the mod's 0.01 typo.
                if item then item:set_condition(name == 'rep_s6' and floor == 0.85 and 0.01 or floor) end
            end
        end
        return class_table
    end
    env.stanok = {baliaaa = bench(0.75), pochinka2 = bench(0.85)}
    env.xr_effects = {
        esc_direction_fire = function(actor)
            calls.tips[#calls.tips + 1] = 'esc_direction_fire'
        end,
        -- Both of these sit behind a timer with no target section, so the mod re-runs them every update.
        actor_enemy = function(actor, npc)
            calls.declared = (calls.declared or 0) + 1
            npc.enemy = true
        end,
        run_postprocess = function(actor, npc, p)
            calls.effectors = (calls.effectors or 0) + 1
        end,
        play_snd = function(actor, npc, p) calls.played[#calls.played + 1] = p[1] end,
        dt_final_rndm_tainik = function() calls.stash = (calls.stash or 0) + 1 end,
        enable_ui = function() calls.ui_enabled = (calls.ui_enabled or 0) + 1 end
    }
    env.task = {completed = "completed", fail = "fail", in_progress = "in_progress"}
    env.game_object = {level_path = "level_path", enemy = "enemy"}
    env.xr_remark = {set_scheme = function(npc, ini, scheme, section)
        env.db.storage[npc:id()] = env.db.storage[npc:id()] or {}
        -- set_scheme rebuilds the switch list on every activation, which is what makes appending safe.
        env.db.storage[npc:id()][scheme] = {section = section, logic = {}}
        return 'remark set'
    end}
    env.Frect = function()
        return {set = function(self, ...) return {...} end}
    end
    env.sound_object = {s2d = "s2d"}
    env.mil_tasks = {
        bloodsuckers_dead = function() return false end,
        lukash_job_fail = function() return false end
    }
    env.gulag_military = {checkStalker = function(community, kind)
        -- What the mod ships: the job lists still name freedom, the table was re-skinned to monolith.
        return community == "monolith"
    end}
    env.dolina_skripts = {esti_art_gemchyg = function() error("unpatched artefact check") end,
        spawn_sirena = function() error("unpatched sirena spawn") end}
    env.sr_timer = {
        set_scheme = function(object, ini, scheme)
            env.db.storage[object:id()] = env.db.storage[object:id()] or {}
            env.db.storage[object:id()][scheme] = {string = "st_time_till_surge-->"}
            return "timer set"
        end,
        parse_data = function(object, text) return {parsed = text} end
    }
    env.level_tasks = {set_task_state = function(state, id, objective)
        calls.task_states[#calls.task_states + 1] = {state = state, id = id, objective = objective}
    end}
    env.level = {
        vertex_id = function(position) return 'lvid:' .. position.x end,
        map_remove_object_spot = function(id, kind) calls.spots[#calls.spots + 1] = {id, kind} end,
        main_input_receiver = function() return calls.receiver end,
        start_stop_menu = function(window, flag) calls.menu_toggles[#calls.menu_toggles + 1] = {window, flag} end
    }
    env.ui_events = {WINDOW_LBUTTON_DB_CLICK = 9}
    env.dialogs = {
        relocate_item_section = function(_, section, direction)
            calls.given[#calls.given + 1] = {section = section, direction = direction}
        end
    }
    env.sak = {
        inventory = {},
        have_item_namber = function(section, count) return (env.sak.inventory[section] or 0) >= count end,
        out_item_namber = function(section, count)
            calls.taken[#calls.taken + 1] = section
            env.sak.inventory[section] = (env.sak.inventory[section] or 0) - count
        end
    }
    -- Every Fort branch pairs give_fort with exactly one reward action, in that order.
    local function fort_reward(name)
        return function() calls.given[#calls.given + 1] = {section = name, direction = 'in'} end
    end
    env.pochinka = {
        b_mne_mod_ekz58 = function() error('unpatched exoskeleton reward') end,
        give_fort = function() error('unpatched fort handover') end,
        esc_l_d_mex_nam_fort_m1 = fort_reward('wpn_fort_mod1'),
        esc_l_d_mex_nam_fort_m2 = fort_reward('wpn_fort_mod2'),
        esc_l_d_mex_nam_fort_m3 = fort_reward('wpn_fort_mod3'),
        esc_l_d_mex_dengi_za_fort = fort_reward('money'),
        esc_l_d_mex_nam_fort_m12 = fort_reward('wpn_fort_mod12'),
        esc_l_d_mex_nam_fort_m22 = fort_reward('wpn_fort_mod22')
    }
    env.dialogs_yantar = {give_ecolog_outfit = function() error('unpatched ecologist reward') end}
    env.has_alife_info = function(name) return calls.infos[name] == true end
    env.decor = {
        spawn_und_tv = function() error('unpatched underground decoration') end,
        spawn_esc_svet_y_sidora = function() error('unpatched campfire light') end
    }
    env.amk = {
        chibi_pereves = function() calls.statics.chibi_pereves = {stale = true} end,
        remove_items = function() error('unpatched item removal') end,
        spawn_item = function(section, position, gv, lv)
            calls.amk_spawns[#calls.amk_spawns + 1] = {section, position, gv, lv}
        end
    }
    -- Fvector is exported with a default constructor only; any argument must raise, as in the engine.
    env.vector = function(...)
        if select('#', ...) > 0 then error('No matching overload found, candidates: vector()') end
        return {set = function(self, x, y, z) self.x, self.y, self.z = x, y, z return self end}
    end
    env.treasure_manager = {CTreasure = {
        give_treasure = function(self, key)
            calls.treasure[#calls.treasure + 1] = key
            self.treasure_info[key].done = true
            return 'granted'
        end
    }}
    env.action = function(object, look_action) calls.looks[#calls.looks + 1] = {object, look_action} end
    env.look = setmetatable({point = 'point'}, {__call = function(_, kind, position)
        return {kind = kind, position = position}
    end})
    env.cond = setmetatable({look_end = 'look_end'}, {__call = function(_, kind) return kind end})
    env.mob_remark = {
        set_scheme = function(npc, ini, scheme, section)
            -- Mirrors utils.cfg_get_number with the mod's string default.
            local storage = {}
            env.db.storage[npc:id()] = env.db.storage[npc:id()] or {}
            env.db.storage[npc:id()][scheme] = storage
            storage.look = ini:line_exist(section, 'target') and
                (tonumber(ini:r_string(section, 'target')) or 0) or ''
            return 'scheme set'
        end,
        mob_remark = {reset_scheme = function(self)
            if self.st.look then
                -- The engine binding rejects a string here; this is the abort the players hit.
                if type(self.st.look) ~= 'number' then error('story_object: no matching overload') end
                calls.looks[#calls.looks + 1] = {self.object, {story = self.st.look}}
            end
            return 'reset'
        end}
    }
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
    env.sound_theme = {ph_snd_themes = {
        unchanged = {"sentinel"},
        -- Classification is by the files a theme plays, so these stand in for the real tables.
        melnica_radio = {[[music\trek_ruba]]},
        kasseta_mysic_3 = {[[music\na_berlin]]},
        radio_sikret4 = {[[radio\hunter_record_1]]},
        bar_start_megafon = {[[characters_voice\scenario\bar\start_1]]}
    }}
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
    env.xr_motivator = {motivator_binder = {
        update = function(_, delta) calls.npc_update = delta end
    }}
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
    -- Sealed stash boxes, the burglar skill, deaths out of sight and the killer-less death branch.
    calls.online, calls.given_infos, calls.switches = {}, {}, {}
    env.treasure_manager.manager = {treasure_by_target = {}, treasure_info = {}}
    env.treasure_manager.get_treasure_manager = function() return env.treasure_manager.manager end
    env.ph_idle = {set_scheme = function(npc, ini, scheme, section)
        env.db.storage[npc:id()] = env.db.storage[npc:id()] or {}
        env.db.storage[npc:id()][scheme] = {nonscript_usable = false, tips = "sealed", section = section}
        npc:set_tip_text("sealed")
        return "idle set"
    end}
    env.ph_door = {set_scheme = function(npc, ini, scheme, section)
        env.db.storage[npc:id()] = env.db.storage[npc:id()] or {}
        env.db.storage[npc:id()][scheme] = {section = section, on_use = npc.existing_use}
        return "door set"
    end}
    env.mob_death = {mob_death = {death_callback = function(self, victim, who)
        assert(who, "unpatched nil killer")
        calls.mob_deaths = (calls.mob_deaths or 0) + 1
        return "mob death"
    end}}
    env.xr_logic.try_switch_to_another_section = function(victim, storage, actor)
        calls.switches[#calls.switches + 1] = {victim = victim, storage = storage, actor = actor}
    end
    -- Story-id conditions: the mod takes the game down when the object a restrictor names is gone.
    calls.conditions = {}
    env.xr_logic.cfg_get_two_strings_and_condlist = function(ini, section, field, npc)
        local row = calls.conditions[field]
        if not row then return nil end
        return {name = field, v1 = row[1], v2 = row[2],
            condlist = env.xr_logic.parse_condlist(npc, section, field, row[3])}
    end
    env.xr_logic.cfg_get_npc_and_zone = function(ini, section, field, npc)
        local condition = env.xr_logic.cfg_get_two_strings_and_condlist(ini, section, field, npc)
        if not condition then return nil end
        local server = env.alife():story_object(tonumber(condition.v1))
        if not server then error("there is no object with story_id '" .. condition.v1 .. "'") end
        condition.npc_id = server.id
        return condition
    end
    env.level.object_by_id = function(id) return calls.online[id] end
    calls.pstor = {}
    env.xr_logic.pstor_retrieve = function(_, name, default) return calls.pstor[name] or default end
    env.xr_logic.pstor_store = function(_, name, value) calls.pstor[name] = value end
    env.bind_monster.generic_object_binder.net_spawn = function(self)
        calls.monster_spawns = (calls.monster_spawns or 0) + 1
        return self.spawn_result ~= false
    end
    env.bind_monster.generic_object_binder.net_destroy = function() return "destroyed" end
    env.xr_motivator.motivator_binder.net_spawn = function(self) return self.spawn_result ~= false end
    env.xr_motivator.motivator_binder.net_destroy = function() return "destroyed" end
    env.delete.remove_by_script = function(id)
        objects[id] = nil
        return "removed"
    end
    env.xr_effects.remove_object = function(_, object) objects[object:id()] = nil end
    env.dead_city = {exterminate_nacsamlet = function() end}
    env.ogsm_mutants = {safely_destroy_creature = function() end, MutantManager = {update = function() end}}
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
            position = function() return {sub = function() return "direction" end} end,
            level_vertex_id = function() return 4242 end,
            game_vertex_id = function() return 42 end,
            id = function() return 0 end,
            give_info_portion = function(_, name)
                calls.infos[name] = true
                calls.given_infos[#calls.given_infos + 1] = name
            end,
            give_game_news = function(_, text) calls.news[#calls.news + 1] = text end
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
    -- The removal guard wraps every delete.* function, so the existing callback is reached through it.
    equal(env.delete.esc_ydalaem_bbbbbbbbbbbbbtttttttttttrrrrr("btr"), "addon",
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
    local env, calls, objects, module, actor = fixture()
    module.install()
    local victim = {id = function() return 47 end, position = function() return {} end,
        hit = function(self, value) calls.impulse = value end}
    local binder = {object = victim, st = {mob_death = {}, active_section = "active", active_scheme = "scheme",
        scheme = {}}}
    env.db.actor = actor()
    -- 65535 is the engine's "belongs to no smart terrain" id, which is what the mod's own function checks.
    objects[47] = {id = 47, smart_terrain_id = function() return 65535 end}
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

do
    local env, calls, _, module = fixture()
    module.install()

    -- The modernisation reward was cloned from the mod15 branch, so the mod58 exo had no producer.
    env.pochinka.b_mne_mod_ekz58(nil, {})
    equal(calls.given[1].section, "outfit_exo_mod58", "the exoskeleton branch grants its own variant")
    equal(calls.given[1].direction, "in", "the exoskeleton is granted, not taken")

    -- Sakharov's reward named a texture path; alife():create fatals on a section that does not exist.
    env.db.actor = {give_info_portion = function(_, name) calls.infos[name] = true end}
    env.dialogs_yantar.give_ecolog_outfit(nil, {})
    equal(calls.given[2].section, "ecolog_outfit", "the ecologist suit is a real item section")
    equal(calls.infos.yan_ecolog_outfit_given, true, "the reward info portion is still granted")
    local granted = #calls.given
    env.dialogs_yantar.give_ecolog_outfit(nil, {})
    equal(#calls.given, granted, "the ecologist suit is granted only once")
end

do
    local env, calls, _, module = fixture()
    module.install()
    env.sak.inventory = {wpn_fort = 1, wpn_fort_mod1 = 1, wpn_fort_mod2 = 1}

    -- Base Fort branches keep their original behaviour: the plain pistol is the one handed in.
    env.pochinka.give_fort()
    env.pochinka.esc_l_d_mex_nam_fort_m1()
    equal(calls.taken[1], "wpn_fort", "the base upgrade still consumes the plain Fort")
    equal(calls.given[#calls.given].section, "wpn_fort_mod1", "the base upgrade still grants mod1")

    -- The two added tiers upgrade an already modified pistol, which is what must be consumed.
    env.pochinka.give_fort()
    env.pochinka.esc_l_d_mex_nam_fort_m12()
    equal(calls.taken[2], "wpn_fort_mod1", "the mod12 upgrade consumes the mod1 pistol")
    equal(env.sak.inventory.wpn_fort, 0, "the plain Fort is no longer destroyed by an upgrade")
    env.pochinka.give_fort()
    env.pochinka.esc_l_d_mex_nam_fort_m22()
    equal(calls.taken[3], "wpn_fort_mod2", "the mod22 upgrade consumes the mod2 pistol")

    -- A reward reached without give_fort must not consume anything.
    local taken = #calls.taken
    env.pochinka.esc_l_d_mex_nam_fort_m12()
    equal(#calls.taken, taken, "a reward without a handover consumes nothing")
end

do
    local env, calls, _, module = fixture()
    module.install()

    -- vector(99) raised before amk.spawn_item was entered, killing the rest of the info portion's actions.
    env.decor.spawn_und_tv()
    equal(#calls.amk_spawns, 1, "the underground television is spawned")
    equal(calls.amk_spawns[1][1], "televizor", "the spawned section is unchanged")
    equal(calls.amk_spawns[1][2].x, -46.8, "the original position is preserved")
    equal(calls.amk_spawns[1][3], 741, "the original game vertex is preserved")
    equal(calls.amk_spawns[1][4], 4350, "the original level vertex is preserved")

    -- Direct dialog grants bypassed CTreasure:use, whose done flag is the only guard against a second spawn.
    local manager = {treasure_info = {esc_secret_truck_goods = {done = false}}}
    equal(env.treasure_manager.CTreasure.give_treasure(manager, "esc_secret_truck_goods"), "granted",
        "the first grant behaves exactly as before")
    equal(env.treasure_manager.CTreasure.give_treasure(manager, "esc_secret_truck_goods"), nil,
        "a second grant of the same stash does nothing")
    equal(#calls.treasure, 1, "the stash contents and map spot are created once")
    equal(env.treasure_manager.CTreasure.give_treasure(manager, "absent"), nil, "an unknown stash is ignored")
end

do
    local env, calls, _, module = fixture()
    module.install()
    local npc = {id = function() return 7 end}
    local function ini(target)
        return {
            section_exist = function() return true end,
            line_exist = function(_, _, key) return key == "target" and target ~= nil end,
            r_string = function() return target end
        }
    end

    -- A section without "target" produced the string "", and story_object("") aborted the scheme.
    equal(env.mob_remark.set_scheme(npc, ini(nil), "mob_remark", "mob_remark2"), "scheme set",
        "the original set_scheme result is retained")
    local state = env.db.storage[7].mob_remark
    equal(state.look, nil, "an absent target leaves no story id behind")
    local instance = {object = "dog", st = state}
    equal(env.mob_remark.mob_remark.reset_scheme(instance), "reset", "a target-less section no longer aborts")
    equal(#calls.looks, 0, "nothing is looked at when no target is configured")

    -- "actor" degraded to story id 0 through atof, so the mob never turned to face the player.
    env.mob_remark.set_scheme(npc, ini("actor"), "mob_remark", "mob_remark")
    state = env.db.storage[7].mob_remark
    equal(state.look, nil, "the actor target is not mistaken for story id 0")
    equal(state.ild_look_actor, true, "the actor target is recognised")
    env.db.actor = {position = function() return "actor_position" end}
    env.mob_remark.mob_remark.reset_scheme({object = "dog", st = state})
    equal(#calls.looks, 1, "the mob turns towards the actor")
    equal(calls.looks[1][2].position, "actor_position", "it looks at the actor's position")

    -- A real story id keeps going through the mod's own look, untouched.
    env.mob_remark.set_scheme(npc, ini("310"), "mob_remark", "mob_remark")
    state = env.db.storage[7].mob_remark
    equal(state.look, 310, "a numeric target is preserved")
    equal(state.ild_look_actor, false, "a numeric target is not treated as the actor")
    env.mob_remark.mob_remark.reset_scheme({object = "burer", st = state})
    equal(calls.looks[2][2].story, 310, "the original story-object look still runs")
end

do
    local env, calls, _, module = fixture()
    module.install()

    -- A missing sound file is a fatal inside the constructor, so the mod's own "if snd_obj then" is too late.
    local silent = env.xr_sound.get_safe_sound_object([[new\ost_mgnovenia_17]])
    check(silent ~= nil, "an absent sound yields a usable stub instead of a fatal")
    equal(#calls.sounds, 0, "the engine constructor is never reached for an absent file")
    silent:play_no_feedback(nil, nil, 0, nil, 1.0)
    silent:play_at_pos(nil, nil, 0)
    equal(silent:playing(), false, "the stub reports that it is not playing")
    check(env.xr_sound.get_safe_sound_object([[soundtrack\controller\7]]) ~= nil,
        "the whole absent soundtrack directory is covered")
    -- Present files must still go to the engine untouched.
    local real = env.xr_sound.get_safe_sound_object([[ambient\da_beep]])
    equal(calls.sounds[1], [[ambient\da_beep]], "present sounds still reach the engine")
    real:play_no_feedback()
    equal(real.played, true, "a real sound object is returned unchanged")
    -- A name the mod mistyped is corrected to the file it meant, so the player still hears it.
    env.xr_sound.get_safe_sound_object([[device\pda_news]])
    equal(calls.sounds[2], [[device\pda\pda_news]], "the gold-fish stash reaches the PDA sound it names")
end

do
    local env, calls, _, module = fixture()
    module.install()
    local npc = {id = function() return 11 end}
    local ini = {r_string = function(_, _, key) return key == "path_main" and "esc_voen_sniper_spati" or nil end}

    -- Inside a gulag the absolute path became esc_blokpost_esc_voen_sniper_spati, which does not exist.
    equal(env.xr_sleeper.set_scheme(npc, ini, "sleeper", "sleeper@esc_blockpost_camper_day", "esc_blokpost"),
        "sleeper set", "the original sleeper result is retained")
    equal(env.db.storage[11].sleeper.path_main, "esc_voen_sniper_spati",
        "the sniper sleeps on the path that actually exists")
    -- Any other gulag path keeps its prefix.
    local other = {r_string = function() return "sleep3" end}
    env.xr_sleeper.set_scheme(npc, other, "sleeper", "sleeper@esc_blockpost_sleeptt", "esc_blokpost")
    equal(env.db.storage[11].sleeper.path_main, "esc_blokpost_sleep3", "ordinary gulag paths are untouched")

    -- An unlisted community has no drop table; pairs(nil) aborted the whole death callback.
    local manager = {npc = {character_community = function() return "actor_freedom" end}}
    equal(env.death_manager.drop_manager.create_release_item(manager), nil,
        "an unlisted community no longer aborts the death callback")
    local known = {npc = {character_community = function() return "stalker" end}}
    equal(env.death_manager.drop_manager.create_release_item(known), "dropped",
        "a listed community still generates its drop")
end

do
    local env, calls, _, module = fixture()
    module.install()
    env.clock_ms = 3600000

    -- m_endTime is compared against a seconds clock, so the divisor has to be exactly 1000.
    env.new_life.stalkerok_navuk_lvl1()
    equal(calls.statics.chibi_rechi_navuk_lvl1.m_endTime, 3600 + 20, "the skill banner expires 20 s from now")
    env.new_life.chibi_dost_za_vse_kassetu()
    equal(calls.statics.chibi_dost_za_vse_kassetu.m_endTime, 3600 + 15, "the cassette banner expires 15 s from now")
    env.new_life.stalkerok_mexanik_navuk_lvl2()
    equal(calls.statics.chibi_mex_navuk_lvl2.m_endTime, 3600 + 20, "the mechanic banner expires 20 s from now")
    env.amk.chibi_pereves()
    equal(calls.statics.chibi_pereves.m_endTime, 3600 + 10, "the overweight icon expires 10 s from now")
    -- The whole point: the timestamp must move with the clock, not drift away from it.
    env.clock_ms = 7200000
    env.amk.chibi_pereves()
    equal(calls.statics.chibi_pereves.m_endTime, 7200 + 10, "the icon still expires 10 s after a later trigger")
end

do
    local env, calls, _, module = fixture()
    module.install()
    local conditions = {}
    local function slot_item(value)
        return {
            condition = function() return value end,
            set_condition = function(self, next) value = next conditions[#conditions + 1] = next end
        }
    end
    env.db.actor = {slots = {}, item_in_slot = function(self, slot) return self.slots[slot] end}

    -- A bench guarantees its tier as a floor; it must not grind a better weapon down to it.
    env.db.actor.slots[1] = slot_item(0.95)
    env.stanok.baliaaa.rep_s1()
    equal(conditions[#conditions], 0.95, "the level-1 bench leaves a better weapon alone")
    env.db.actor.slots[2] = slot_item(0.30)
    env.stanok.baliaaa.rep_s2()
    equal(conditions[#conditions], 0.75, "the level-1 bench still repairs a worn weapon to its tier")
    env.db.actor.slots[1] = slot_item(0.95)
    env.stanok.pochinka2.rep_s1()
    equal(conditions[#conditions], 0.95, "the level-2 bench leaves a better weapon alone")
    -- The level-2 slot-6 handler carries a 0.01 typo that destroyed the outfit outright.
    env.db.actor.slots[6] = slot_item(0.60)
    env.stanok.pochinka2.rep_s6()
    equal(conditions[#conditions], 0.85, "the level-2 bench no longer ruins the outfit it should repair")
end

do
    local env, calls, _, module = fixture()
    module.install()

    -- The mod dropped this vanilla string id, and the engine returns the id itself when it is absent.
    env.xr_effects.esc_direction_fire({}, {})
    equal(#calls.tips, 0, "no raw string id is pushed to the PDA")
    calls.strings = {esc_direction_fire = "Fire!"}
    env.xr_effects.esc_direction_fire({}, {})
    equal(#calls.tips, 1, "a translated string is still sent")

    -- The campfire light was attached to a graph vertex 619 m away, so it never switched online.
    env.db.actor = {game_vertex_id = function() return 55 end}
    env.decor.spawn_esc_svet_y_sidora()
    local light = calls.created[#calls.created]
    equal(light[1], "svet_kostra", "the campfire light section is unchanged")
    equal(light[2].x, -242, "the original position is preserved")
    equal(light[3], "lvid:-242", "the level vertex is resolved from that position")
    equal(light[4], 55, "the game vertex is the level the actor is actually on")
end

do
    local env, calls, _, module = fixture()
    module.install()
    local function play(theme)
        local source = {st = {theme = theme}, played_sound = {volume = 1.0}}
        env.ph_sound.snd_source.update(source, 10)
        return source.played_sound.volume
    end

    -- Classification comes from the theme's own files, so a source the name list never mentioned still
    -- follows the setting, while speech and machinery keep the mod's own volume.
    equal(play("melnica_radio"), 0.7, "a music path is recognised")
    equal(play("kasseta_mysic_3"), 0.7, "a music source follows the setting")
    equal(play("radio_sikret4"), 0.7, "a named radio theme is recognised")
    equal(play("bar_start_megafon"), 1.0, "speech is left alone")
    equal(calls.sound_updates, 4, "the original scheme update always runs")

    -- The setting is re-read, but not on every frame.
    calls.settings.radio_volume = "0"
    equal(play("melnica_radio"), 0.7, "the value is cached rather than read every update")
    env.clock_ms = env.clock_ms + 2000
    equal(play("melnica_radio"), 0, "zero mutes the source completely")
    calls.settings.radio_volume = "100"
    env.clock_ms = env.clock_ms + 2000
    equal(play("radio_yroveni1"), 1, "full volume is restored")
    calls.settings.radio_volume = ""
    env.clock_ms = env.clock_ms + 2000
    equal(play("melnica_radio"), 0.7, "an unset value falls back to the default")

    -- A source that never started playing must not be touched.
    local silent = {st = {theme = "melnica_radio"}}
    env.ph_sound.snd_source.update(silent, 10)
    equal(silent.played_sound, nil, "a source with no sound is left as it is")
end


-- The NPC stall watchdog. Every fault below leaves a scheme waiting on a callback the engine will never
-- deliver, which is what strands an NPC mid-quest until the player reloads.
do
    local env, calls, _, module = fixture()
    module.install()
    env.game_object = {level_path = "level_path", enemy = "enemy"}
    env.move = {dodge = "dodge", walk = "walk", standing = "standing"}

    -- Each scenario gets its own NPC: the watchdog remembers one record per id, as it does in the game.
    local next_id = 6
    local function make_world(overrides)
        overrides = overrides or {}
        next_id = next_id + 1
        local npc_id = next_id
        local mgr = {
            state = 1,
            path_walk = "walk_path",
            path_walk_info = {},
            path_look = "look_path",
            path_look_info = {[0] = {}},
            last_look_index = 0,
            last_index = 0,
            current_point_index = 0,
            team = nil,
            suggested_state = {},
            resets = 0,
            callbacks = 0,
            signals = {},
            patrol_walk = {
                count = function() return 3 end,
                point = function(_, index) return {x = 10 * index, y = 0, z = 0} end,
                level_vertex_id = function(_, index) return 100 + index end
            }
        }
        mgr.reset = function(self) self.resets = self.resets + 1 end
        mgr.time_callback = function(self) self.callbacks = self.callbacks + 1 end
        mgr.scheme_set_signal = function(self, name) self.signals[#self.signals + 1] = name end
        local states = {callback = {func = function() end, timeout = 5000}}
        local npc = {
            x = 0,
            id = function() return npc_id end,
            name = function() return "esc_stalker" end,
            alive = function() return true end,
            position = function(self) return {x = self.x, y = 0, z = 0,
                distance_to = function(point) return math.abs(point.x - self.x) end} end,
            best_enemy = function() return overrides.enemy end,
            best_danger = function() return nil end,
            is_talking = function() return overrides.talking == true end,
            get_current_point_index = function() return 1 end,
            animation_count = function(self) return self.animations or 0 end,
            clear_animations = function(self) self.animations = 0 end,
            set_body_state = function(self, kind) self.body_state = kind end,
            set_path_type = function(self, kind) self.path_type = kind end,
            set_detail_path_type = function(self, kind) self.detail_type = kind end,
            set_movement_type = function(self, kind) self.movement_type = kind end,
            set_dest_level_vertex_id = function(self, vertex) self.dest_vertex = vertex end
        }
        env.db.storage[npc_id] = {
            active_scheme = "walker",
            move_mgr = mgr,
            state_mgr = states,
            walker = {section = "walker@guard", path_walk = "walk_path"}
        }
        return npc, mgr, states
    end

    local function tick(npc, seconds)
        env.clock_ms = env.clock_ms + seconds * 1000
        env.xr_motivator.motivator_binder.update({object = npc}, 10)
    end

    -- Movement that never happens: the patrol is rebuilt twice, and only then is the NPC placed on the
    -- point it was walking to.
    local npc, mgr = make_world()
    tick(npc, 1)
    tick(npc, 15)
    equal(mgr.resets, 0, "a walking NPC is left alone before the grace period")
    tick(npc, 10)
    equal(mgr.resets, 1, "a stalled walk is rebuilt once the grace period passes")
    tick(npc, 25)
    equal(mgr.resets, 2, "a second rebuild follows if the first changed nothing")
    equal(mgr.last_index, nil, "the second attempt forgets the remembered waypoint")
    equal(mgr.current_point_index, nil, "and the remembered patrol index with it")
    tick(npc, 25)
    equal(npc.dest_vertex, 101, "a route that cannot be walked is replanned as a free level path")
    equal(npc.path_type, "level_path", "and the NPC is taken off the patrol path to walk it")
    equal(npc.movement_type, "walk", "on foot, so the detour looks like the walk it replaces")
    equal(npc.body_state, "standing", "and it is stood up first, in case an animation is what pinned it")
    check(calls.console and calls.console[#calls.console] == "ild_update watchdog rescue esc_stalker walker@guard",
        "every intervention is recorded")

    -- Reaching the point hands the scheme's own patrol straight back.
    npc.x = 10
    tick(npc, 5)
    equal(mgr.resets, 3, "arriving at the point restores the scheme's own patrol")
    check(calls.console[#calls.console] == "ild_update watchdog rejoined esc_stalker walker@guard",
        "and the hand-back is recorded too")

    -- Progress resets everything: an NPC that is actually walking is never touched.
    npc, mgr = make_world()
    for _ = 1, 20 do
        npc.x = npc.x + 1
        tick(npc, 5)
    end
    equal(mgr.resets, 0, "an NPC that keeps moving is never interfered with")

    -- Combat, conversation and a meet in progress are legitimate reasons to stand still.
    npc, mgr = make_world({enemy = {}})
    for _ = 1, 10 do tick(npc, 5) end
    equal(mgr.resets, 0, "an NPC holding position in combat is left alone")

    npc, mgr = make_world({talking = true})
    for _ = 1, 10 do tick(npc, 5) end
    equal(mgr.resets, 0, "an NPC talking to the player is left alone")

    npc, mgr = make_world()
    env.db.storage[npc:id()].meet = {meet_manager = {state = "wait"}}
    for _ = 1, 10 do tick(npc, 5) end
    equal(mgr.resets, 0, "an NPC meeting the player is left alone")

    -- A finite wait whose animation never arrived: the state manager never arms callback.begin, so the
    -- wait that ends the section never starts. Re-running the wait is what the game itself would do.
    local states
    npc, mgr, states = make_world()
    mgr.state = 2
    tick(npc, 1)
    tick(npc, 20)
    equal(mgr.callbacks, 0, "a wait that has only just started is left to run")
    npc.animations = 1
    tick(npc, 35)
    equal(mgr.callbacks, 1, "a wait whose animation never arrived is re-run")
    equal(npc.animations, 0, "and the animation that pinned the NPC is dropped with it")

    -- A wait that did arm is the engine's to finish, and so is a wait of "*", which carries no callback.
    npc, mgr, states = make_world()
    mgr.state = 2
    states.callback.begin = env.clock_ms
    for _ = 1, 10 do tick(npc, 5) end
    equal(mgr.callbacks, 0, "an armed wait is left to the engine")

    npc, mgr, states = make_world()
    mgr.state = 2
    states.callback.func = nil
    for _ = 1, 10 do tick(npc, 5) end
    equal(mgr.callbacks, 0, "a wait of '*' ends on a signal and is never forced")

    -- A team signal whose partners never became ready holds the whole group forever.
    npc, mgr = make_world()
    mgr.state = 2
    mgr.syn_signal = "sync_done"
    tick(npc, 1)
    tick(npc, 30)
    equal(#mgr.signals, 0, "a team signal is given time to resolve on its own")
    tick(npc, 70)
    equal(mgr.signals[1], "sync_done", "a team signal that never resolved is issued")
    equal(mgr.syn_signal, nil, "and is not issued twice")

    -- A scheme the movement manager is not running must never be touched by it.
    npc, mgr = make_world()
    env.db.storage[npc:id()].walker.path_walk = "another_path"
    for _ = 1, 10 do tick(npc, 5) end
    equal(mgr.resets, 0, "a movement manager running someone else's path is left alone")

    npc, mgr = make_world()
    env.db.storage[npc:id()].active_scheme = nil
    for _ = 1, 10 do tick(npc, 5) end
    equal(mgr.resets, 0, "an NPC with no active scheme is left alone")

    -- The original binder still runs.
    npc = make_world()
    tick(npc, 1)
    equal(calls.npc_update, 10, "the engine's own binder update still runs")
end

local function make_ini(sections)
    return {
        section_exist = function(_, section) return sections[section] ~= nil end,
        line_exist = function(_, section, key) return sections[section] ~= nil and sections[section][key] ~= nil end,
        r_string = function(_, section, key) return sections[section][key] end
    }
end

local function make_physic(id, cfg, extra)
    local object = {
        id = function() return id end,
        name = function() return "object" .. id end,
        spawn_ini = function() return make_ini({logic = {cfg = cfg}}) end,
        set_nonscript_usable = function(self, value) self.usable = value end,
        set_tip_text = function(self, text) self.tip = text end
    }
    for key, value in pairs(extra or {}) do object[key] = value end
    return object
end

-- Sealed treasure boxes: the box Sidorovich sells, and any other box sealed with the mod's "never" idiom, is
-- searchable once its treasure has been granted, and stays exactly as the mod left it before that.
do
    local env, calls, objects, module, actor = fixture()
    module.install()
    env.db.actor = actor()
    local manager = env.treasure_manager.manager
    manager.treasure_by_target[5018] = "esc_secret_truck_goods"
    manager.treasure_info.esc_secret_truck_goods = {target = 5018, done = false}
    objects[300] = {id = 300, m_story_id = 5018}
    local sealed = make_physic(300, [[scripts\tainiki\esc_orig_tainik_zakrut.ltx]])
    equal(env.ph_idle.set_scheme(sealed, "ini", "ph_idle", "ph_idle"), "idle set", "the idle scheme result is retained")
    equal(sealed.usable, nil, "a sealed box whose treasure was never granted stays sealed")
    equal(sealed.tip, "sealed", "and keeps the mod's own tip")
    manager.treasure_info.esc_secret_truck_goods.done = true
    env.ph_idle.set_scheme(sealed, "ini", "ph_idle", "ph_idle")
    equal(sealed.usable, true, "a granted sealed box becomes searchable when its scheme starts")
    equal(sealed.tip, "st_search_treasure", "with the treasure box's own tip")
    equal(env.db.storage[300].ph_idle.nonscript_usable, true, "the scheme state agrees, so a reset keeps it open")
    equal(env.db.storage[300].ph_idle.tips, "st_search_treasure", "and keeps the tip through a reset")
    objects[301] = {id = 301, m_story_id = 5018}
    local plain = make_physic(301, [[scripts\treasure_inventory_box.ltx]])
    env.ph_idle.set_scheme(plain, "ini", "ph_idle", "ph_idle")
    equal(plain.usable, nil, "an ordinary box is left to its own configuration")
    objects[302] = {id = 302, m_story_id = 5011}
    local decoy = make_physic(302, [[scripts\tainiki\esc_orig_tainik_zakrut.ltx]])
    env.ph_idle.set_scheme(decoy, "ini", "ph_idle", "ph_idle")
    equal(decoy.usable, nil, "a sealed box nobody was pointed at stays sealed")
    objects[303] = {id = 303, m_story_id = -1}
    local unnamed = make_physic(303, [[scripts\tainiki\esc_orig_tainik_zakrut.ltx]])
    env.ph_idle.set_scheme(unnamed, "ini", "ph_idle", "ph_idle")
    equal(unnamed.usable, nil, "a box without a story id is not looked up")
end

-- A box that is already online when its treasure is granted opens right away, not on the next load.
do
    local env, calls, objects, module, actor = fixture()
    module.install()
    env.db.actor = actor()
    objects[310] = {id = 310, m_story_id = 5018}
    calls.story = {[5018] = objects[310]}
    local sealed = make_physic(310, [[scripts\tainiki\esc_orig_tainik_zakrut.ltx]])
    env.ph_idle.set_scheme(sealed, "ini", "ph_idle", "ph_idle")
    calls.online[310] = sealed
    local manager = {treasure_info = {esc_secret_truck_goods = {target = 5018, done = false}}}
    equal(env.treasure_manager.CTreasure.give_treasure(manager, "esc_secret_truck_goods"), "granted",
        "the grant result is retained")
    equal(sealed.usable, true, "the box the player just paid for opens at once")
    equal(sealed.tip, "st_search_treasure", "and shows the search tip")
    calls.online[310] = nil
    sealed.usable = nil
    manager.treasure_info.esc_secret_truck_goods.done = false
    env.treasure_manager.CTreasure.give_treasure(manager, "esc_secret_truck_goods")
    equal(sealed.usable, nil, "an offline box waits for its scheme to start")
    objects[311] = {id = 311, m_story_id = 5044}
    calls.story[5044] = objects[311]
    local ordinary = make_physic(311, [[scripts\treasure_inventory_box.ltx]])
    env.ph_idle.set_scheme(ordinary, "ini", "ph_idle", "ph_idle")
    calls.online[311] = ordinary
    manager.treasure_info.gar_secret_toilet = {target = 5044, done = false}
    env.treasure_manager.CTreasure.give_treasure(manager, "gar_secret_toilet")
    equal(ordinary.usable, nil, "an ordinary granted box is not touched")
end

-- The burglar skill: both journals stand in for the level-1 portion that nothing ever grants.
do
    local env, _, _, module = fixture()
    module.install()
    local tikhon = make_physic(600, [[scripts\kordon\dveri_tixona_podsobka.ltx]])
    local both = "{+esc_jyrnal2_s_navuk +val_jyrnal_vzlom2} ph_door@locked"
    local parsed = env.xr_logic.parse_condlist(tikhon, "ph_door@locked0", "on_info", "{+navuk_vzlom_lvl1} ph_door@locked")
    equal(parsed.source, both, "Tikhon's storeroom waits for both burglar journals")
    equal(parsed.npc, tikhon, "the door object is retained")
    equal(parsed.field, "on_info", "the field is retained")
    equal(env.xr_logic.parse_condlist(tikhon, "ph_door@locked0", "on_info", " {+navuk_vzlom_lvl1}  ph_door@locked ").source,
        both, "insignificant whitespace does not hide the condition")
    equal(env.xr_logic.parse_condlist(tikhon, "ph_door@locked", "on_use", "{+navuk_vzlom_lvl1} ph_door@locked").source,
        "{+navuk_vzlom_lvl1} ph_door@locked", "other fields of the door are unchanged")
    local other = make_physic(601, [[scripts\kordon\dveri_v_derevne1.ltx]])
    equal(env.xr_logic.parse_condlist(other, "ph_door@locked0", "on_info", "{+navuk_vzlom_lvl1} ph_door@locked").source,
        "{+navuk_vzlom_lvl1} ph_door@locked", "other doors are unchanged")
    equal(env.xr_logic.parse_condlist(nil, "ph_door@locked0", "on_info", "{+navuk_vzlom_lvl1} ph_door@locked").source,
        "{+navuk_vzlom_lvl1} ph_door@locked", "nil-object parsing is forwarded unchanged")

    local safe = make_physic(602, [[scripts\barr\avtpark_seif.ltx]])
    equal(env.ph_door.set_scheme(safe, "ini", "ph_door", "ph_door@locked"), "door set", "the door scheme result is retained")
    local state = env.db.storage[602].ph_door
    equal(state.on_use.name, "on_use", "the autopark safe gains the use it was missing")
    equal(state.on_use.condlist.source,
        "{+esc_jyrnal2_s_navuk +val_jyrnal_vzlom2} %+bar_avtoparka_seif_otkrulsa% ph_door@open",
        "which opens it on the skill and records that it opened")
    equal(state.on_use.condlist.section, "ph_door@locked", "parsed for the locked section")
    equal(state.on_use.condlist.npc, safe, "parsed for the safe itself")
    env.ph_door.set_scheme(safe, "ini", "ph_door", "ph_door@open")
    equal(env.db.storage[602].ph_door.on_use, nil, "the safe's other sections keep their own configuration")
    local kept = make_physic(603, [[scripts\barr\avtpark_seif.ltx]], {existing_use = "existing"})
    env.ph_door.set_scheme(kept, "ini", "ph_door", "ph_door@locked")
    equal(env.db.storage[603].ph_door.on_use, "existing", "a use the configuration already has is never replaced")
end

-- The X18 terminal's transition names a section that does not exist, which aborts on the door code.
do
    local env, _, _, module = fixture()
    module.install()
    local terminal = {name = function() return "lab_psihoz113" end}
    local broken = "{+lab_minys_psiz} sr_idle2"
    local fixed = "{+lab_minys_psiz} ph_idle2"
    equal(env.xr_logic.parse_condlist(terminal, "ph_idle", "on_info", broken).source, fixed,
        "the terminal falls silent through the empty section its own file declares")
    equal(env.xr_logic.parse_condlist(terminal, "ph_idle", "on_info", "  {+lab_minys_psiz}sr_idle2 ").source, fixed,
        "however the ini spaced it")
    equal(env.xr_logic.parse_condlist(terminal, "ph_idle2", "on_info", broken).source, broken,
        "the object's other sections are untouched")
    equal(env.xr_logic.parse_condlist(terminal, "ph_idle", "on_use", broken).source, broken,
        "and its other fields are untouched")
    local other = {name = function() return "lab_psihoz112" end}
    equal(env.xr_logic.parse_condlist(other, "ph_idle", "on_info", broken).source, broken,
        "another object with the same section keeps its own line")
    equal(env.xr_logic.parse_condlist(nil, "ph_idle", "on_info", broken).source, broken,
        "nil-object parsing is forwarded unchanged")
end

-- The ghost-house window restrictor has no unconditional else, so it aborts on every later entry to Cordon.
do
    local env, _, _, module = fixture()
    module.install()
    local window = make_physic(700, [[scripts\kordon_new\space_okno_prizrakdoma.ltx]])
    local broken = "{-esc_dom_prizraka_kiknem} sr_idle"
    local fixed = "{-esc_dom_prizraka_kiknem} sr_idle, nil"
    equal(env.xr_logic.parse_condlist(window, "logic", "active", broken).source, fixed,
        "the restrictor falls silent through the same nil its own sections switch to")
    equal(env.xr_logic.parse_condlist(window, "logic", "active", " {-esc_dom_prizraka_kiknem}sr_idle ").source,
        fixed, "however the ini spaced it")
    equal(env.xr_logic.parse_condlist(window, "sr_idle", "on_info", broken).source, broken,
        "its other sections are untouched")
    local other = make_physic(701, [[scripts\kordon_new\dom_prizrak_pechka.ltx]])
    equal(env.xr_logic.parse_condlist(other, "logic", "active", broken).source, broken,
        "another restrictor keeps its own line")
    -- Lis's camper section was renamed and one transition kept pointing at the old name.
    local fox = "{+escape_stalker_done !_used} camper@esc_stalker_fox"
    equal(env.xr_logic.parse_condlist(nil, "remark@esc_stalker_fox", "on_info", fox).source,
        "{+escape_stalker_done !_used} camper@esc_stalker_foxik", "and the dangling camper target is repaired")
    equal(env.xr_logic.parse_condlist(nil, "camper@esc_stalker_fox1", "on_info", fox).source, fox,
        "only in the section that carries it")
    local door = make_physic(604, [[scripts\kordon\dveri_tixona_podsobka.ltx]])
    env.ph_door.set_scheme(door, "ini", "ph_door", "ph_door@locked")
    equal(env.db.storage[604].ph_door.on_use, nil, "other locked doors are unchanged")
end

-- Deaths the quest logic never heard about: a scripted mob's [death] portions are delivered once the mob is
-- found dead or gone, unless a script removed it.
do
    local env, calls, objects, module, actor = fixture()
    module.install()
    env.db.actor = actor()
    local monster = env.bind_monster.generic_object_binder
    local stalker = env.xr_motivator.motivator_binder
    local function make_mob(id, name, sections)
        objects[id] = {id = id, alive = function() return true end}
        return {object = {id = function() return id end, name = function() return name end},
            st = {ini = make_ini(sections), section_logic = "logic"}}
    end
    local function death(line, second)
        return {logic = {active = "mob_home", on_death = "death"}, death = {on_info = line, on_info2 = second}}
    end
    local binder = {object = env.db.actor, first_update = false}
    local function pass(seconds)
        env.clock_ms = env.clock_ms + seconds * 1000
        env.bind_stalker.actor_binder.update(binder, 1)
    end

    local boar = make_mob(400, "esc_kaban_na_tainik11", death("%+esc_kaban_tainika1_dead1%"))
    equal(monster.net_spawn(boar), true, "the mod's spawn result is retained")
    equal(calls.monster_spawns, 1, "the mod's own spawn still runs")
    pass(4)
    equal(#calls.given_infos, 0, "a living mob hands out nothing")
    objects[400].alive = function() return false end
    pass(4)
    equal(calls.given_infos[1], "esc_kaban_tainika1_dead1", "a mob found dead delivers its death portion")
    equal(#calls.given_infos, 1, "exactly once")
    pass(4)
    equal(#calls.given_infos, 1, "and is then forgotten")

    local dog = make_mob(401, "esc_tonnel_psevdodog1",
        death("%+esc_tonnel_psidog1_death =play_snd(monsters\\dog)%", "{+killed_by_actor} %+conditional_portion%"))
    monster.net_spawn(dog)
    objects[401] = nil
    pass(4)
    equal(calls.given_infos[2], "esc_tonnel_psidog1_death", "a mob that vanished out of sight delivers too")
    equal(calls.infos.conditional_portion, nil, "a conditional clause is never forced")

    calls.infos.escp_dead_tyshkan89 = true
    local rat = make_mob(402, "esc_tyshkan89", death("%+escp_dead_tyshkan89%"))
    monster.net_spawn(rat)
    objects[402] = nil
    pass(4)
    equal(#calls.given_infos, 2, "a portion the player already holds is not given again")

    local white = make_mob(403, "gar_myt_v_lesy3", death("%+gar_dopq_myt3_podox +gar_dopq_myt1_podox%"))
    calls.infos.gar_dopq_myt1_podox = true
    monster.net_spawn(white)
    objects[403].alive = function() return false end
    pass(4)
    equal(calls.given_infos[3], "gar_dopq_myt3_podox", "only the portions still missing are delivered")
    equal(#calls.given_infos, 3, "the one already held is skipped")

    local guard = make_mob(404, "esc_last_day_oxr1", death("%+guard_dead%"))
    equal(stalker.net_spawn(guard), true, "the stalker binder is watched the same way")
    equal(env.delete.remove_by_script(404), "removed", "the mod's removal result is retained")
    pass(4)
    equal(calls.infos.guard_dead, nil, "an NPC removed by the mod's own script is not reported dead")

    local released = make_mob(405, "released", death("%+released_dead%"))
    monster.net_spawn(released)
    objects[405] = nil
    equal(monster.net_destroy(released), "destroyed", "the mod's net_destroy result is retained")
    pass(4)
    equal(calls.infos.released_dead, nil, "a release seen at net_destroy is not a death")

    local offline = make_mob(406, "offline", death("%+offline_dead%"))
    monster.net_spawn(offline)
    monster.net_destroy(offline)
    pass(4)
    equal(calls.infos.offline_dead, nil, "an offline switch is not a death")
    objects[406].alive = function() return false end
    pass(4)
    equal(calls.infos.offline_dead, true, "a death after the offline switch is delivered")

    local effect = make_mob(407, "effect", death("%+effect_dead%"))
    monster.net_spawn(effect)
    env.xr_effects.remove_object("actor", effect.object)
    pass(4)
    equal(calls.infos.effect_dead, nil, "an object removed through remove_object is not a death")

    local late = make_mob(408, "late", death("%+late_dead%"))
    monster.net_spawn(late)
    objects[408] = nil
    pass(1)
    equal(calls.infos.late_dead, nil, "the check runs at most once per period")
    pass(3)
    equal(calls.infos.late_dead, true, "and the next period delivers")

    local plain = make_mob(409, "plain", {logic = {active = "mob_home"}})
    equal(monster.net_spawn(plain), true, "a mob without a death section spawns as before")
    objects[409] = nil
    pass(4)
    equal(#calls.given_infos, 5, "and is never reported")
    local failed = make_mob(410, "failed", death("%+failed_dead%"))
    failed.spawn_result = false
    equal(monster.net_spawn(failed), false, "a failed spawn is returned unchanged")
    objects[410] = nil
    pass(4)
    equal(calls.infos.failed_dead, nil, "and is not watched")
end

-- The Agroprom dossier: nothing ever granted it, so it follows the death portion of the NPC it belongs to.
do
    local env, calls, _, module, actor = fixture()
    module.install()
    env.db.actor = actor()
    local binder = {object = env.db.actor, first_update = false}
    local function pass(seconds)
        env.clock_ms = env.clock_ms + seconds * 1000
        env.bind_stalker.actor_binder.update(binder, 1)
    end
    pass(4)
    equal(calls.infos.agro_vzial_infy_s_terista_naca, nil, "a living terrorist hands nothing over")
    calls.infos.aro_pidr1_strelki_death = true
    pass(4)
    equal(calls.infos.agro_vzial_infy_s_terista_naca, true, "his death hands over the dossier as well")
    local delivered = #calls.given_infos
    pass(4)
    equal(#calls.given_infos, delivered, "and it is handed over exactly once")
end

-- A restrictor that names a story object the mod has already removed must not abort.
do
    local env, calls, _, module = fixture()
    module.install()
    local zone = {name = function() return "esc_gar_dezertiru_sqda_zone" end}
    local function condition(field)
        return env.xr_logic.cfg_get_npc_and_zone("ini", "sr_idle", field, zone)
    end
    calls.conditions.on_npc_in_zone = {"038", "esc_gar_dezertiru_sqda_zone", "%+esc_ydalite_nps_dezov_iz_gar% nil"}
    calls.story = {}
    local absent = condition("on_npc_in_zone")
    equal(absent.name, "on_npc_in_zone", "the condition is kept, so later numbered ones are still read")
    equal(absent.npc_id, 65535, "and points at no object, so it can never fire")
    equal(absent.v2, "esc_gar_dezertiru_sqda_zone", "the zone it names is retained")
    equal(absent.condlist.source, "%+esc_ydalite_nps_dezov_iz_gar% nil", "as is what it would have done")
    calls.story = {[38] = {id = 700}}
    equal(condition("on_npc_in_zone").npc_id, 700, "an object that is still there resolves exactly as before")
    calls.story = {}
    calls.conditions.on_npc_not_in_zone = {"038", "z", "nil"}
    equal(condition("on_npc_not_in_zone").name, "on_npc_in_zone",
        "the not-in-zone form cannot start firing on an object nobody can find")
    calls.conditions.on_npc_not_in_zone2 = {"038", "z", "nil"}
    equal(condition("on_npc_not_in_zone2").name, "on_npc_in_zone2", "and keeps its place in the numbered list")
    equal(condition("on_npc_in_zone9"), nil, "a field the section does not have is still nil")
end

-- smart_terrain.on_death indexes both the dying object and its smart terrain without a check.
do
    local env, calls, objects, module = fixture()
    module.install()
    objects[80] = {id = 80, smart_terrain_id = function() return 12 end}
    objects[12] = {id = 12, gulag = {}}
    env.smart_terrain.on_death(80)
    equal(calls.terrain, 80, "a death inside a live smart terrain still reaches the mod's own handler")
    calls.terrain = nil
    env.smart_terrain.on_death(81)
    equal(calls.terrain, nil, "a death whose server object is already released is not indexed")
    objects[82] = {id = 82, smart_terrain_id = function() return 13 end}
    env.smart_terrain.on_death(82)
    equal(calls.terrain, nil, "nor is one whose smart terrain is gone")
    objects[83] = {id = 83, smart_terrain_id = function() return 65535 end}
    env.smart_terrain.on_death(83)
    equal(calls.terrain, 83, "an object that belongs to no smart terrain still reaches it")
end

-- The ATP scene: its closing portion has one source and nothing in the chain has a timeout.
local function atp_fixture()
    local env, calls, objects, module, actor = fixture()
    module.install()
    env.db.actor = actor()
    local binder = {object = env.db.actor, first_update = false}
    calls.story = {[41] = {id = 41, alive = function() return true end},
        [42] = {id = 42, alive = function() return true end}}
    calls.infos.atp_sdelka_t_fraza1 = true
    return env, calls, objects, function(seconds)
        env.clock_ms = env.clock_ms + seconds * 1000
        env.bind_stalker.actor_binder.update(binder, 1)
    end
end

do
    local env, calls, _, pass = atp_fixture()
    pass(4)
    equal(calls.infos.esc_atp_ydalai_ysex_k_xyiam_end, nil, "a scene that is still running is left alone")
    calls.story[41] = nil
    pass(4)
    equal(calls.infos.esc_atp_ydalai_ysex_k_xyiam_end, nil, "one tick of a missing leader is not yet a stall")
    pass(6)
    equal(calls.infos.esc_atp_ydalai_ysex_k_xyiam_end, true, "a leader who cannot come back closes the scene")
    local delivered = #calls.given_infos
    pass(6)
    equal(#calls.given_infos, delivered, "and it is closed exactly once")
end

do
    local env, calls, _, pass = atp_fixture()
    calls.story[42] = {id = 42, alive = function() return false end}
    calls.infos.atp_sdelka_t_fraza6 = true
    pass(4)
    pass(6)
    equal(calls.infos.atp_sdelka_t_fraza7, true, "a dead Tikhon hands the leader the cue only he could give")
    equal(calls.infos.esc_atp_ydalai_ysex_k_xyiam_end, nil,
        "and the leader still closes the scene himself, so nothing of it is skipped")
end

do
    local env, calls, _, pass = atp_fixture()
    local playing = 1
    calls.online[41] = {id = function() return 41 end, name = function() return "esc_atp_glava_specnaz" end,
        active_sound_count = function() return playing end}
    env.db.storage[41] = {active_scheme = "remark", combat_ignore = {enabled = false},
        remark = {section = "remark4", logic = {{name = "on_signal", v1 = "sound_end"}}}}
    pass(4)
    equal(#calls.switches, 0, "a section that has only just started is not touched")
    pass(95)
    equal(env.db.storage[41].combat_ignore.enabled, true, "combat_ignore is turned back on, as a reload would")
    equal(env.db.storage[41].remark.signals, nil, "a sound that is still playing is left to finish")
    equal(#calls.switches, 1, "and the section is asked to make its own transition")
    playing = 0
    pass(95)
    equal(env.db.storage[41].remark.signals["sound_end"], true, "a wait for a sound that ended is released")
    equal(calls.infos.esc_atp_ydalai_ysex_k_xyiam_end, nil, "no portion is granted behind the scene's back")
end

-- Reba's spy job stamps itself failed at the very moment the actor reports it done.
do
    local env, calls, _, module, actor = fixture()
    module.install()
    env.db.actor = actor()
    local binder = {object = env.db.actor, first_update = false}
    local function pass(seconds)
        env.clock_ms = env.clock_ms + seconds * 1000
        env.bind_stalker.actor_binder.update(binder, 1)
    end
    pass(4)
    equal(#calls.task_states, 0, "a job nobody reported is left alone")
    equal(env.new_life.mne_nagrada_ot_rebu_za_shpionov("actor", "reba"), "desert eagle",
        "the reward the phrase hands out is retained")
    equal(calls.rewards, 1, "and the mod's own reward still runs once")
    equal(calls.infos.esc_reba_proverky_proshel, true, "reporting the kills completes the job the task names")
    equal(calls.infos.esc_reba_proverky_ne_proshel, nil,
        "the fail portion is left to the dialog, which two other dialogs still wait for")
    pass(4)
    equal(#calls.task_states, 2, "and both objectives that name the portion are settled")
    equal(calls.task_states[1].state, "completed", "as completed")
    equal(calls.task_states[1].id, "esc_proverka_na_killerstvo", "on the spy job")
    equal(calls.task_states[1].objective, 0, "the task itself")
    equal(calls.task_states[2].objective, 3, "and its closing objective")
    env.new_life.mne_nagrada_ot_rebu_za_shpionov("actor", "reba")
    pass(4)
    equal(#calls.task_states, 2, "a second run of the phrase settles nothing twice")
end

-- Two effects sit behind a timer with no target section, so the mod re-applies them on every update.
do
    local env, calls, _, module = fixture()
    module.install()
    local spy = {enemy = false,
        relation = function(self) return self.enemy and env.game_object.enemy or "neutral" end}
    env.xr_effects.actor_enemy("actor", spy)
    equal(calls.declared, 1, "the first declaration of war still happens")
    env.xr_effects.actor_enemy("actor", spy)
    env.xr_effects.actor_enemy("actor", spy)
    equal(calls.declared, 1, "and is not repeated once the relation already says so")
    local other = {enemy = false, relation = function() return "neutral" end}
    env.xr_effects.actor_enemy("actor", other)
    equal(calls.declared, 2, "another object still gets its own declaration")
    -- An object this build will not answer for keeps the mod's own unconditional declaration.
    env.xr_effects.actor_enemy("actor", {enemy = false})
    equal(calls.declared, 3, "and so does one with no relation to read")

    env.xr_effects.run_postprocess("actor", nil, {"agr_u_fade"})
    env.xr_effects.run_postprocess("actor", nil, {"agr_u_fade"})
    equal(calls.effectors, 1, "one blackout adds one effector, not one per frame")
    env.xr_effects.run_postprocess("actor", nil, {"other_fade"})
    equal(calls.effectors, 2, "a different effect is never suppressed")
    env.clock_ms = env.clock_ms + 1000
    env.xr_effects.run_postprocess("actor", nil, {"agr_u_fade"})
    equal(calls.effectors, 3, "and the same one runs again when it is genuinely asked for later")
    env.xr_effects.play_snd("actor", nil, {[[new\prilet]]})
    env.xr_effects.play_snd("actor", nil, {[[new\prilet]]})
    equal(#calls.played, 1, "a sound asked for again while it is the same sound is one sound")
    env.xr_effects.play_snd("actor", nil, {[[device\pda\pda_tip]]})
    equal(#calls.played, 2, "a different sound is never suppressed")
end

-- The Agroprom underground betrayal: nothing in the mod ever kills the three soldiers it says died.
do
    local env, calls, _, module, actor = fixture()
    module.install()
    env.db.actor = actor()
    local function soldier(id, section)
        return {id = function() return id end, name = function() return section end,
            section = function() return section end}
    end
    local prapor = soldier(900, "und_prapor2")
    equal(env.xr_remark.set_scheme(prapor, "ini", "remark", "remark@suicide"), "remark set",
        "the mod's own scheme result is retained")
    local logic = env.db.storage[900].remark.logic
    equal(#logic, 1, "the section that says suicide finally has an end")
    equal(logic[1].name, "on_timer1", "as a timer the switch loop already understands")
    equal(logic[1].v1, 12000, "the Prapor falls first, so the scene still plays around him")
    equal(logic[1].condlist.source, "%=kill%", "and dies by his own hand, as the section is named")
    env.xr_remark.set_scheme(soldier(901, "agro_sania"), "remark", "remark", "remark66")
    equal(env.db.storage[901].remark.logic[1].v1, 16000, "the sergeants follow him")
    env.xr_remark.set_scheme(soldier(902, "agro_pania"), "remark", "remark", "remark66")
    equal(env.db.storage[902].remark.logic[1].v1, 20000, "one after the other")
    env.xr_remark.set_scheme(prapor, "ini", "remark", "remark3")
    equal(#env.db.storage[900].remark.logic, 0, "his earlier sections are left exactly as they were")
    env.xr_remark.set_scheme(soldier(903, "agr_soldat_kamp1"), "ini", "remark", "remark66")
    equal(#env.db.storage[903].remark.logic, 0, "and no other remark user is touched")

    local binder = {object = env.db.actor, first_update = false}
    local function pass(seconds)
        env.clock_ms = env.clock_ms + seconds * 1000
        env.bind_stalker.actor_binder.update(binder, 1)
    end
    calls.infos.und_prapor_3 = true
    pass(4)
    equal(#calls.task_states, 0, "the underground task is left alone while the Prapor lives")
    calls.infos.und_prapor_dead = true
    pass(4)
    equal(#calls.task_states, 2, "his death re-asserts the completion the dialog already gave")
    equal(calls.task_states[1].id, "agro_v_podzemky", "on the underground task")
    equal(calls.task_states[1].state, "completed", "as completed, whichever portion the engine reads first")
    equal(#calls.news, 0, "and no hint is shown while the branch still has a next step")
    calls.infos.agro_door_open = true
    pass(4)
    equal(#calls.news, 1, "the dead end finally says where to go")
    equal(calls.infos.agro_finaluch_igra, true, "through a flag the mod declares and never uses")
    pass(4)
    equal(#calls.news, 1, "exactly once")
end

-- Scenes that gate the story behind a sound that may never end, and the last resort when their NPC is gone.
do
    local env, calls, objects, module, actor = fixture()
    module.install()
    env.db.actor = actor()
    local function scene_npc(id, section)
        return {id = function() return id end, name = function() return section end,
            section = function() return section end, active_sound_count = function() return 0 end}
    end
    local commander = scene_npc(920, "b_kom_stroi")
    env.xr_remark.set_scheme(commander, "ini", "remark", "remark2")
    local logic = env.db.storage[920].remark.logic
    equal(logic[1].v1, 25000, "the Bar formation cannot wait on its sound for ever")
    equal(logic[1].condlist.source, "%+bar_krik_konec% remark3",
        "and makes the transition the section itself would have made")
    env.xr_remark.set_scheme(commander, "ini", "remark", "remark")
    equal(#env.db.storage[920].remark.logic, 0, "its earlier sections are untouched")
    local ss = scene_npc(921, "dt_ss_desantnik_kom")
    env.xr_remark.set_scheme(ss, "ini", "remark", "remark1")
    equal(env.db.storage[921].remark.logic[1].condlist.source, "%+ddt_ss_desant_na_start% walker",
        "the paratroop commander starts moving")
    env.xr_remark.set_scheme(ss, "ini", "remark", "remark2")
    equal(env.db.storage[921].remark.logic[1].condlist.source, "%+dt_ss_nakonecto_k_tonelq% camper55",
        "and reaches the only section in which he can be talked to at all")

    local binder = {object = env.db.actor, first_update = false}
    local function pass(seconds)
        env.clock_ms = env.clock_ms + seconds * 1000
        env.bind_stalker.actor_binder.update(binder, 1)
    end
    calls.infos.bar_v_stroi = true
    pass(4)
    equal(calls.infos.bar_krik_konec, nil, "a scene still under way is left alone")
    pass(200)
    equal(calls.infos.bar_krik_konec, true, "a commander who never reports in stops holding up the game")
    calls.infos.ddt_rally_otschet = true
    pass(4)
    equal(calls.infos.dt_ralli_strt, nil, "the countdown is given its own time first")
    pass(50)
    equal(calls.infos.dt_ralli_strt, true, "then the paid-for race starts anyway")

    -- Letyagin: input and HUD come back with the portion, and the stash the section promised is still made.
    calls.infos.bar_letia_final = true
    pass(4)
    equal(calls.ui_enabled, nil, "his walk is given its own time first")
    pass(130)
    equal(calls.infos.barar_vse_letiaga_yshel, true, "then the farewell is closed out")
    equal(calls.ui_enabled, 1, "the input and the HUD come back")
    equal(calls.stash, 1, "and the reward stash the line promised is still made")

    -- Melisa: the only source of the defuser every finale device tests for.
    equal(#calls.created, 0, "nothing is handed over while she is alive")
    local melisa = {object = scene_npc(922, "bar_melisa_nyjen_tyt")}
    objects[922] = {id = 922}
    env.xr_motivator.motivator_binder.net_spawn(melisa)
    pass(4)
    equal(#calls.created, 0, "still nothing while she is alive")
    objects[922] = nil
    pass(4)
    equal(calls.created[1][1], "bar_deaktiv_iaderki", "her loss no longer ends the game")
    equal(calls.infos.bar_melisa_finish, true, "and the dialog she owned counts as done")
    pass(4)
    equal(#calls.created, 1, "exactly once")
end

-- Dark Valley, Yantar and the Military Warehouses: chains whose only producer the mod removed.
do
    local env, calls, objects, module, actor = fixture()
    module.install()
    env.db.actor = actor()
    local binder = {object = env.db.actor, first_update = false}
    local function pass(seconds)
        env.clock_ms = env.clock_ms + seconds * 1000
        env.bind_stalker.actor_binder.update(binder, 1)
    end
    -- The X18 entrance door starts open, so the use that granted these two can never happen.
    pass(4)
    equal(calls.infos.val_x18_door_open, nil, "a player who has not reached the lab gets nothing")
    calls.infos.labx_nachalo_pizdeca = true
    pass(4)
    equal(calls.infos.val_x18_door_open, true, "reaching the lab opens the documents chain")
    equal(calls.infos.dar_run_quest, true, "and gives the task that hangs off it")

    local grate = make_physic(950, [[scripts\yan\yan_grate.ltx]])
    equal(env.xr_logic.parse_condlist(grate, "ph_door@close", "on_info", "{+art_start} ph_door@open").source,
        "{+yan_labx16_switcher_primary_off} ph_door@open_final",
        "the X16 grate opens on the emitter again, as vanilla has it")
    equal(env.xr_logic.parse_condlist(grate, "ph_door@open", "on_info", "{+art_start} ph_door@open").source,
        "{+art_start} ph_door@open", "and its other sections are untouched")

    local sniper = make_physic(951, [[scripts\ded_city\nemec_sniper1_v_kpss_logic.ltx]])
    equal(env.xr_logic.parse_condlist(sniper, "kamp", "on_info", "{!is_night} kamp").source,
        "{!is_night} camper", "the KPSS snipers can return to their nest at dawn")
    equal(env.xr_logic.parse_condlist(sniper, "camper", "on_info", "{=is_night} kamp").source,
        "{=is_night} kamp", "the way out to the campfire is unchanged")

    equal(env.mil_tasks.bloodsuckers_dead(), false, "the hunt is not finished by walking in")
    calls.infos.mil_village_bloodsucker1 = true
    calls.infos.mil_village_bloodsucker2 = true
    calls.infos.mil_village_bloodsucker3 = true
    equal(env.mil_tasks.bloodsuckers_dead(), true,
        "but clearing the village stands in for four monsters the spawn no longer holds")
    equal(env.gulag_military.checkStalker("freedom", "mil_village"), true,
        "Freedom can fill the jobs its own profiles are named in")
    equal(env.gulag_military.checkStalker("monolith", "mil_village"), true, "and the mod's own answer stands")
    equal(env.gulag_military.checkStalker("bandit", "mil_village"), false, "nobody else is let in")
    equal(env.mil_tasks.lukash_job_fail(), true, "an order against a group that is not placed is reported failed")
    calls.story = {[702] = {id = 702}, [708] = {id = 708}}
    equal(env.mil_tasks.lukash_job_fail(), false, "and left alone where the targets do exist")
    equal(env.dolina_skripts.esti_art_gemchyg(), false, "Mazai still wants the step before his last")
    calls.infos.val_kom_mazai_4 = true
    equal(env.dolina_skripts.esti_art_gemchyg(), true, "which the player can actually reach")
end

-- The endgame: one sound, one blackout and one counter stand between the player and the ending.
do
    local env, calls, objects, module, actor = fixture()
    module.install()
    env.db.actor = actor()
    local binder = {object = env.db.actor, first_update = false}
    local function pass(seconds)
        env.clock_ms = env.clock_ms + seconds * 1000
        env.bind_stalker.actor_binder.update(binder, 1)
    end
    calls.pstor.mon_destroy_generator = 5
    pass(60)
    equal(calls.infos.sar_monolith_destroy, nil, "five generators is not six")
    calls.pstor.mon_destroy_generator = 6
    pass(4)
    equal(calls.infos.sar_monolith_destroy, nil, "and the alarm is given its own time")
    pass(50)
    equal(calls.infos.sar_monolith_destroy, true, "then the only way into the last room opens anyway")
    equal(calls.infos.sar_monolith_off, true, "with the portion that came in the same breath")

    calls.infos.bun_actor_fall = true
    pass(4)
    equal(calls.ui_enabled, nil, "the blackout is allowed to play out")
    pass(130)
    equal(calls.infos.bun_antenna_off, true, "then the chain that was owed is paid")
    equal(calls.infos.sar_monolith_call, true, "including the call that opens the Monolith task")
    equal(calls.ui_enabled, 1, "and the player gets the controls back")

    local guards = {}
    for index = 1, 5 do
        local id = 960 + index
        objects[id] = {id = id, alive = function() return true end}
        guards[index] = {object = {id = function() return id end,
            name = function() return string.format("mon_stalker_walker4_%04d", index) end}}
        env.xr_motivator.motivator_binder.net_spawn(guards[index])
    end
    pass(4)
    equal(calls.pstor.mon_clear_control_room, nil, "living guards keep the door shut")
    for index = 1, 4 do objects[960 + index].alive = function() return false end end
    pass(4)
    equal(calls.pstor.mon_clear_control_room, nil, "four of five is not enough")
    objects[965].alive = function() return false end
    pass(4)
    equal(calls.pstor.mon_clear_control_room, 5, "all five dead opens the way to the generator hall")
    calls.pstor.mon_clear_control_room = 9
    pass(4)
    equal(calls.pstor.mon_clear_control_room, 9, "and a counter that is already high is never touched")

    local timer = {id = function() return 970 end, name = function() return "aes_space_restrictor_timer" end}
    equal(env.sr_timer.set_scheme(timer, "ini", "sr_timer", "sr_timer1"), "timer set",
        "the mod's own timer scheme still runs")
    equal(env.db.storage[970].sr_timer.on_value.parsed, "0|nil",
        "the surge counter that was commented out can now leave the HUD")
end

-- The killer-less death branch of mob_death writes to a local only the other branch declares.
do
    local env, calls, _, module, actor = fixture()
    module.install()
    env.db.actor = actor()
    local victim = {id = function() return 500 end, name = function() return "victim" end}
    env.db.storage[500] = {}
    local instance = {object = victim, st = {section = "death"}}
    equal(env.mob_death.mob_death.death_callback(instance, victim, {id = function() return 1 end}), "mob death",
        "a known killer keeps the mod's callback")
    equal(calls.mob_deaths, 1, "which ran once")
    equal(env.mob_death.mob_death.death_callback(instance, victim, nil), nil, "an unknown killer no longer raises")
    equal(env.db.storage[500].death.killer, -1, "and is recorded the way the scheme intends")
    equal(env.db.storage[500].death.killer_name, nil, "with no killer name")
    equal(calls.switches[1].victim, victim, "the death section still runs its transitions")
    equal(calls.switches[1].storage, instance.st, "for the scheme's own storage")
    equal(calls.switches[1].actor, env.db.actor, "with the actor")
    equal(calls.mob_deaths, 1, "without going through the broken branch")
end

print("script_repairs_tests: " .. tests .. " checks passed")
