local source_path = arg[1] or "payload/gamedata/scripts/ild_mod_repairs.script"
local tests = 0

local function check(value, message)
    assert(value, message)
    tests = tests + 1
end

local function equal(actual, expected, message)
    check(actual == expected, message .. ": expected " .. tostring(expected) .. ", got " .. tostring(actual))
end

-- The engine compiles every script inside its own namespace, so the module is loaded into a table of its own and
-- the globals it reads are supplied here rather than by the desktop interpreter.
local function fixture()
    local calls = {console = {}, logged = {}}
    local env = setmetatable({}, {__index = _G})
    env._G = env
    env.db = {actor = nil, storage = {}}
    env.get_console = function()
        return {execute = function(_, line) calls.console[#calls.console + 1] = line end}
    end
    env.log = function(text) calls.logged[#calls.logged + 1] = text end
    -- luabind's class "name" (base) registers a callable table in the script's environment; the crow binder
    -- the repairs install is declared that way.
    env.object_binder = {}
    env.super = function() end
    env.class = function(name)
        env[name] = setmetatable({}, {__call = function(class_table, ...)
            local object = setmetatable({}, {__index = class_table})
            if class_table.__init then class_table.__init(object, ...) end
            return object
        end})
        return function() return env[name] end
    end
    local chunk = assert(loadfile(source_path))
    setfenv(chunk, env)
    chunk()
    return env, calls
end

do
    local env, calls = fixture()
    -- Every repair in this file installs behind pcall, so a module the mod does not have on a given save costs
    -- that one repair and nothing else. This is what keeps the pack from ever being the reason a game dies.
    env.install()
    check(true, "install() completes with none of the mod's modules present")
    equal(#calls.console, 0, "installing writes nothing to the console")
    env.install()
    check(true, "installing twice is a no-op")
end

-- Locks opened by hand: the section a key, lockpick or tool led to is remembered in the actor's pstor and
-- replayed on the object's first update after a reload, where the mod's own logic would lock it again.
do
    local env, calls = fixture()
    local pstor, switched, updates, fatal = {}, {}, {}, {}
    env.level = {name = function() return calls.level or "l01_escape" end}
    env.xr_logic = {
        pstor_store = function(_, name, value) pstor[name] = value end,
        pstor_retrieve = function(_, name, default) return pstor[name] or default end,
        switch_to_section = function(object, state, section)
            switched[#switched + 1] = {object:name(), section}
            env.db.storage[object:id()].active_section = section ~= "nil" and section or nil
        end
    }
    -- The engine's own reader: a key the section lacks is a fatal that no pcall in the script can catch, so
    -- the stand-in records it as one and the test fails on any record, whatever the script did with it.
    local function ini(lines)
        local sections = {}
        for name in pairs(lines) do sections[string.match(name, "^(.-):")] = true end
        return {
            r_string = function(_, section, key)
                local value = lines[section .. ":" .. key]
                if value == nil then fatal[#fatal + 1] = "Can't find variable " .. key .. " in [" .. section .. "]" end
                return value
            end,
            line_exist = function(_, section, key) return lines[section .. ":" .. key] ~= nil end,
            section_exist = function(_, section) return sections[section] == true end
        }
    end
    -- The mod's own use: the section the state names as its outcome becomes active.
    local function use_stub(self) env.db.storage[self.object:id()].active_section = self.st.outcome end
    local function update_stub(self) updates[#updates + 1] = self.object:name() end
    env.ph_door = {action_door = {use_callback = use_stub, update = update_stub}}
    env.ph_idle = {action_idle = {use_callback = use_stub, update = update_stub}}
    env.db.actor = {}
    env.install()
    local door_ini = ini({
        ["ph_door@locked:on_use"] = "{=actor_has_item(agro_klqchi_ot_bashni)} "
            .. "%=remove_item(agro_klqchi_ot_bashni)% ph_door@open",
        ["ph_door@open:on_use"] = "ph_door@close", ["ph_door@close:on_use"] = "ph_door@open"
    })
    local function object(id, name) return {id = function() return id end, name = function() return name end} end
    local hatch = object(77, "agro_lqk_na_vushky")
    env.db.storage[77] = {active_section = "ph_door@locked"}
    local action = {object = hatch, st = {ini = door_ini, locked = true, outcome = "ph_door@open"}}
    env.ph_door.action_door.update(action, 1)
    equal(#updates, 1, "a session with nothing remembered runs the door's own update")
    env.ph_door.action_door.use_callback(action, hatch, env.db.actor)
    equal(env.db.storage[77].active_section, "ph_door@open", "the key opens the hatch as the mod wrote it")
    equal(pstor["ild_lock_l01_escape:agro_lqk_na_vushky"], "ph_door@open",
        "and the section it led to is remembered, under the level and the name")
    -- Closing and opening the unlocked hatch by hand is not a lock being opened.
    action.st.locked, action.st.outcome = false, "ph_door@close"
    env.ph_door.action_door.use_callback(action, hatch, env.db.actor)
    equal(pstor["ild_lock_l01_escape:agro_lqk_na_vushky"], "ph_door@open", "a plain toggle changes nothing remembered")
    -- The next session: the object comes back locked, with the key long gone.
    env.db.storage[77] = {active_section = "ph_door@locked"}
    local reloaded = {object = hatch, st = {ini = door_ini, locked = true}}
    env.ph_door.action_door.update(reloaded, 1)
    equal(switched[1] and switched[1][2], "ph_door@open", "the remembered section is replayed on the first update")
    equal(#updates, 1, "in place of that update")
    env.ph_door.action_door.update(reloaded, 1)
    equal(#updates, 2, "and never again in the session")
    equal(#switched, 1, "nor a second time")

    -- A box released with a lockpick ends its logic; it comes back released.
    local box_ini = ini({["ph_idle:on_use"] = "{=actor_has_item(otmuchki) ~40} %=minys_otmuchka% nil"})
    local box = object(78, "gar_tainik_s_artami")
    env.db.storage[78] = {active_section = "ph_idle"}
    local use = function(module, item, state) module.use_callback({object = item, st = state}, item, env.db.actor) end
    use(env.ph_idle.action_idle, box, {ini = box_ini, outcome = nil})
    equal(pstor["ild_lock_l01_escape:gar_tainik_s_artami"], "nil", "a released box is remembered as released")
    env.db.storage[78] = {active_section = "ph_idle"}
    env.ph_idle.action_idle.update({object = box, st = {ini = box_ini}}, 1)
    equal(switched[2][2], "nil", "and released again after a reload")
    equal(env.db.storage[78].active_section, nil, "with its logic gone")

    -- A stove lit with a match burns down on its own timer and is lit each time; nothing to remember.
    local stove_ini = ini({
        ["ph_idle:on_use"] = "{=actor_has_item(item_spichki) ~7} %=remove_item(item_spichki)% ph_idle1",
        ["ph_idle1:on_use"] = "{+aiaiaiai}", ["ph_idle1:on_timer"] = "55000 | ph_idle"
    })
    local stove = object(79, "dom_prizrak_pechka")
    env.db.storage[79] = {active_section = "ph_idle"}
    use(env.ph_idle.action_idle, stove, {ini = stove_ini, outcome = "ph_idle1"})
    equal(pstor["ild_lock_l01_escape:dom_prizrak_pechka"], nil, "a section that times out is not remembered")

    -- The car hood taken apart with a tool ends in the never-again idiom and stays taken apart.
    local hood_ini = ini({["ph_idle:on_use"] = "{=actor_has_item(instrymentu_tonk)} %=give_items_to_actor(x)% ph_idle2",
        ["ph_idle2:on_use"] = "{+aiaiaiai}"})
    local hood = object(80, "agr_zapor_kapot")
    env.db.storage[80] = {active_section = "ph_idle"}
    use(env.ph_idle.action_idle, hood, {ini = hood_ini, outcome = "ph_idle2"})
    equal(pstor["ild_lock_l01_escape:agr_zapor_kapot"], "ph_idle2", "a use that ends in the never-again idiom is remembered")

    -- A use that leads nowhere - the lockpick that snapped - records nothing.
    env.db.storage[81] = {active_section = "ph_idle"}
    local stuck = object(81, "stuck_box")
    use(env.ph_idle.action_idle, stuck, {ini = box_ini, outcome = "ph_idle"})
    equal(pstor["ild_lock_l01_escape:stuck_box"], nil, "a failed attempt leaves nothing behind")
    equal(#calls.console, 0, "none of it touches the console")
    equal(#fatal, 0, "and no section was asked for a line it lacks: " .. table.concat(fatal, "; "))

    -- Sections with no on_use at all, as the mod's files really have them: the code-locked box in
    -- [ph_idle@enable], the camp door whose open section has only a timer, the factory door whose open
    -- section has nothing. 1.0.10 read the line unasked and every one of these was a fatal on F.
    local coded = ini({["ph_idle@enable:tips"] = "x"})
    env.db.storage[82] = {active_section = "ph_idle@enable"}
    use(env.ph_idle.action_idle, object(82, "dak"), {ini = coded, outcome = "ph_idle@enable"})
    local camp = ini({["ph_door@close1:on_use"] = "ph_door@open1", ["ph_door@open1:on_timer"] = "5000 | ph_door@close1"})
    env.db.storage[83] = {active_section = "ph_door@close1"}
    env.ph_door.action_door.use_callback({object = object(83, "meln_konclager_dver"),
        st = {ini = camp, locked = false, outcome = "ph_door@open1"}}, nil, env.db.actor)
    local factory = ini({["ph_door@close2:on_use"] = "ph_door@open"})
    env.db.storage[84] = {active_section = "ph_door@close2"}
    env.ph_door.action_door.use_callback({object = object(84, "dveri_v_zavode"),
        st = {ini = factory, locked = false, outcome = "ph_door@open"}}, nil, env.db.actor)
    equal(#fatal, 0, "no line is read that is not there: " .. table.concat(fatal, "; "))
    equal(pstor["ild_lock_l01_escape:dak"], nil, "a box with nothing to remember is not remembered")
    equal(pstor["ild_lock_l01_escape:meln_konclager_dver"], nil, "nor a door on its way back")
    equal(pstor["ild_lock_l01_escape:dveri_v_zavode"], nil, "nor a door that spends nothing")
    env.db.storage[84] = {active_section = "ph_door@close2"}
    env.ph_door.action_door.update({object = object(84, "dveri_v_zavode"), st = {ini = factory}}, 1)
    equal(#fatal, 0, "nor on a replay")

    -- Keys 1.0.10 wrote under the bare name: the Cordon box's namesakes on other levels share them, so one is
    -- followed only where this object's own lock leads to that very section.
    calls.level = "l02_garbage"
    pstor.ild_lock_gar_tainik_s_artami = "ph_idle2"
    local namesake_ini = ini({["ph_idle:on_use"] = "{=actor_has_item(otmuchki) ~40} %=minys_otmuchka% nil"})
    env.db.storage[85] = {active_section = "ph_idle"}
    local before = #switched
    env.ph_idle.action_idle.update({object = object(85, "gar_tainik_s_artami"), st = {ini = namesake_ini}}, 1)
    equal(#switched, before, "a section this box does not have is not entered")
    pstor.ild_lock_gar_tainik_s_artami = "nil"
    env.db.storage[86] = {active_section = "ph_idle"}
    env.ph_idle.action_idle.update({object = object(86, "gar_tainik_s_artami"), st = {ini = namesake_ini}}, 1)
    equal(switched[#switched][2], "nil", "the release a lockpick box's own use leads to is followed")
    pstor.ild_lock_agro_lqk_na_vushky = "ph_door@open"
    calls.level = "l03_agroprom"
    env.db.storage[87] = {active_section = "ph_door@locked"}
    env.ph_door.action_door.update({object = object(87, "agro_lqk_na_vushky"), st = {ini = door_ini, locked = true}}, 1)
    equal(switched[#switched][2], "ph_door@open", "and so is the hatch's open section, which its key leads to")
    pstor.ild_lock_some_plain_door = "ph_door@open"
    env.db.storage[88] = {active_section = "ph_door@close"}
    before = #switched
    env.ph_door.action_door.update({object = object(88, "some_plain_door"), st = {ini = door_ini, locked = false}}, 1)
    equal(#switched, before, "a door that is no lock is left as the level brought it")
    equal(#fatal, 0, "with no line read that is not there")
end

-- The six-button panel in X18: a press switches its two ring neighbours exactly once, and a panel a save
-- already holds out of the solvable class is put back to the shipped position before the first press.
do
    local initial = {"ph_button@vkl", "ph_button@vkl", "ph_button@vukl", "ph_button@vkl", "ph_button@vukl", "ph_button@vkl"}
    local function panel_fixture(sections, solved)
        local env, calls = fixture()
        local infos, disabled, switches = {}, {}, {}
        if solved then infos.labx_smska_test_bliati = true end
        env.has_alife_info = function(name) return infos[name] == true end
        env.db.actor = {disable_info_portion = function(_, name)
            infos[name] = nil
            disabled[#disabled + 1] = name
        end}
        local servers, buttons = {}, {}
        for index = 1, 6 do
            local id = 700 + index
            servers["lab_konpka_paneli0" .. index] = {id = id}
            buttons[id] = {id = function() return id end, name = function() return "lab_konpka_paneli0" .. index end}
            env.db.storage[id] = {active_scheme = "ph_button", active_section = sections[index], ph_button = {}}
        end
        env.alife = function() return {object = function(_, key) return servers[key] end} end
        env.level = {object_by_id = function(id) return buttons[id] end, name = function() return "l04u_labx18" end}
        env.xr_logic = {
            switch_to_section = function(object, state, section)
                switches[#switches + 1] = {object:name(), section}
                env.db.storage[object:id()].active_section = section
            end,
            pstor_store = function() end, pstor_retrieve = function(_, _, default) return default end
        }
        -- The mod's own press: the portion is granted and the button switches itself.
        env.ph_button = {ph_button = {
            use_callback = function(self)
                local storage = env.db.storage[self.object:id()]
                infos["labx_knopka" .. string.match(self.object:name(), "(%d)$")] = true
                storage.active_section = storage.active_section == "ph_button@vkl" and "ph_button@vukl" or "ph_button@vkl"
                return "pressed"
            end,
            update = function() return "updated" end
        }}
        env.ph_door = {action_door = {use_callback = function() end, update = function() end}}
        env.ph_idle = {action_idle = {use_callback = function() end, update = function() end}}
        env.install()
        local function press(index) return env.ph_button.ph_button.use_callback({object = buttons[700 + index]}) end
        local function update(index) return env.ph_button.ph_button.update({object = buttons[700 + index]}) end
        return env, infos, disabled, switches, press, update
    end

    local env, infos, disabled, switches, press, update = panel_fixture(initial)
    equal(update(1), "updated", "the mod's own update runs")
    equal(#switches, 0, "the shipped position is left as it is")
    equal(press(1), "pressed", "the mod's own press runs")
    equal(env.db.storage[701].active_section, "ph_button@vukl", "and toggles the button pressed")
    equal(#switches, 2, "its two ring neighbours are switched by the pack, once each")
    equal(switches[1][1], "lab_konpka_paneli02", "the second")
    equal(switches[1][2], "ph_button@vukl", "out")
    equal(switches[2][1], "lab_konpka_paneli05", "and the fifth")
    equal(switches[2][2], "ph_button@vkl", "on")
    equal(infos.labx_knopka1, nil, "and the press portion is withdrawn before any update could read it")
    equal(disabled[1], "labx_knopka1", "by name")
    press(6)
    for index = 1, 6 do
        equal(env.db.storage[700 + index].active_section, "ph_button@vukl", "1 and 6 from the shipped position put button " .. index .. " out")
    end
    equal(#switches, 4, "four neighbours switched for two presses")
    press(3)
    equal(env.db.storage[702].active_section, "ph_button@vkl", "a press lights its neighbours again")
    equal(env.db.storage[704].active_section, "ph_button@vkl", "both of them")
    equal(env.db.storage[703].active_section, "ph_button@vkl", "and itself")

    -- A panel the timing race knocked out of the solvable class: one button lit alone.
    local knocked = {"ph_button@vkl", "ph_button@vukl", "ph_button@vukl", "ph_button@vukl", "ph_button@vukl", "ph_button@vukl"}
    local kenv, kinfos, kdisabled, kswitches, _, kupdate = panel_fixture(knocked)
    kinfos.labx_knopka3 = true
    kupdate(4)
    equal(#kswitches, 3, "the buttons that differ from the shipped position are put back")
    for index = 1, 6 do
        equal(kenv.db.storage[700 + index].active_section, initial[index], "button " .. index .. " as shipped")
    end
    equal(kinfos.labx_knopka3, nil, "and a stray press portion goes with them")
    equal(#kdisabled, 6, "every one of the six is withdrawn")
    kupdate(4)
    equal(#kswitches, 3, "once")

    -- A solvable position other than the shipped one is nobody's business.
    local solvable = {"ph_button@vukl", "ph_button@vukl", "ph_button@vukl", "ph_button@vkl", "ph_button@vkl", "ph_button@vkl"}
    local senv, _, _, sswitches, _, supdate = panel_fixture(solvable)
    supdate(2)
    equal(#sswitches, 0, "a position the rule of threes can reach is left alone")
    -- And a panel already solved is never touched.
    local _, _, _, dswitches, _, dupdate = panel_fixture(knocked, true)
    dupdate(1)
    equal(#dswitches, 0, "nor a panel that has already done its work")
end

-- The safe in the death tunnel hands its use to the keypad hidden inside its own model.
do
    local env = fixture()
    local infos, events, used = {}, {}, 0
    env.has_alife_info = function(name) return infos[name] == true end
    env.db.actor = {}
    local lock = {id = function() return 90 end, name = function() return "esc_toneli_smertiseif_zamok" end}
    env.db.storage[90] = {active_scheme = "ph_code", ph_code = {actions = {}}}
    env.alife = function() return {object = function(_, key) return key == "esc_toneli_smertiseif_zamok" and {id = 90} or nil end} end
    env.level = {object_by_id = function(id) return id == 90 and lock or nil end, name = function() return "l01_escape" end}
    env.xr_logic = {
        issue_event = function(object, state, event) events[#events + 1] = {object, state, event} end,
        pstor_store = function() end, pstor_retrieve = function(_, _, default) return default end,
        switch_to_section = function() end
    }
    env.ph_idle = {action_idle = {use_callback = function() used = used + 1 return "used" end, update = function() end}}
    env.ph_door = {action_door = {use_callback = function() end, update = function() end}}
    env.install()
    local function ini() return {line_exist = function() return false end, section_exist = function() return false end} end
    local safe = {object = {section = function() return "esc_toneli_smerti_seif" end, id = function() return 91 end,
        name = function() return "safe" end}, st = {ini = ini()}}
    env.db.storage[91] = {active_section = "ph_idle"}
    equal(env.ph_idle.action_idle.use_callback(safe, safe.object, env.db.actor), nil,
        "a use of the safe without the code goes to the keypad")
    equal(#events, 1, "as the keypad's own use")
    equal(events[1][1], lock, "on the lock")
    equal(events[1][2], env.db.storage[90].ph_code, "with its scheme storage")
    equal(events[1][3], "use_callback", "and its use event")
    equal(used, 0, "the safe's own use does not run")
    infos.estonsmerti_seif_open = true
    equal(env.ph_idle.action_idle.use_callback(safe, safe.object, env.db.actor), "used",
        "with the code entered the safe opens as written")
    local crate = {object = {section = function() return "some_crate" end, id = function() return 92 end,
        name = function() return "crate" end}, st = {ini = ini()}}
    env.db.storage[92] = {active_section = "ph_idle"}
    equal(env.ph_idle.action_idle.use_callback(crate, crate.object, env.db.actor), "used", "any other box is untouched")
    equal(#events, 1, "and asks nothing of the keypad")
end

-- The hatch to the Agroprom tower: a key spent before locks were remembered leaves the diary out of reach.
do
    local function hatch_fixture(setup)
        local env = fixture()
        local infos, pstor, objects, updates = {spawn_vse_agro_dekoru = true}, {}, {}, 0
        env.has_alife_info = function(name) return infos[name] == true end
        env.xr_logic = {
            pstor_store = function(_, name, value) pstor[name] = value end,
            pstor_retrieve = function(_, name, default) return pstor[name] or default end,
            switch_to_section = function() end
        }
        env.db.actor = {}
        env.alife = function() return {object = function(_, id) return objects[id] end} end
        env.level = {name = function() return "l03_agroprom" end, object_by_id = function() return nil end}
        env.bind_stalker = {actor_binder = {net_spawn = function() return true end,
            update = function() updates = updates + 1 return "updated" end}}
        env.ph_idle = {action_idle = {use_callback = function() end, update = function() end}}
        env.ph_door = {action_door = {use_callback = function() end, update = function() end}}
        if setup then setup(infos, pstor, objects) end
        env.install()
        return env, pstor, function() return env.bind_stalker.actor_binder.update({}, 1) end
    end
    local key = "ild_lock_l03_agroprom:agr_m_dog_e_0000"
    local env, pstor, tick = hatch_fixture()
    for _ = 1, 15 do equal(tick(), "updated", "the mod's own update runs") end
    equal(pstor[key], nil, "the whole simulation is searched for the key first")
    tick()
    equal(pstor[key], "ph_door@open", "with no key anywhere, the hatch is remembered open")
    local _, keyed, keyed_tick = hatch_fixture(function(_, _, objects)
        objects[40000] = {section_name = function() return "agro_klqchi_ot_bashni" end}
    end)
    for _ = 1, 17 do keyed_tick() end
    equal(keyed[key], nil, "a key still in the world - in its box or the rucksack - leaves everything as it is")
    local _, read, read_tick = hatch_fixture(function(infos) infos.agro_infa_na_vushke = true end)
    for _ = 1, 17 do read_tick() end
    equal(read[key], nil, "nor is anything done once the diary has been read")
    local _, kept, kept_tick = hatch_fixture(function(_, pstor) pstor.ild_lock_agr_m_dog_e_0000 = "ph_door@open" end)
    for _ = 1, 17 do kept_tick() end
    equal(kept[key], nil, "a hatch 1.0.10 remembered by name needs no second record")
    local _, early, early_tick = hatch_fixture(function(infos) infos.spawn_vse_agro_dekoru = nil end)
    for _ = 1, 17 do early_tick() end
    equal(early[key], nil, "and nothing is done before the tower's decorations exist")
end


print("PASS " .. tests .. " checks")
