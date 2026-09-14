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
    local pstor, switched, updates = {}, {}, {}
    env.xr_logic = {
        pstor_store = function(_, name, value) pstor[name] = value end,
        pstor_retrieve = function(_, name, default) return pstor[name] or default end,
        switch_to_section = function(object, state, section)
            switched[#switched + 1] = {object:name(), section}
            env.db.storage[object:id()].active_section = section ~= "nil" and section or nil
        end
    }
    local function ini(lines)
        return {
            r_string = function(_, section, key) return lines[section .. ":" .. key] end,
            line_exist = function(_, section, key) return lines[section .. ":" .. key] ~= nil end
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
    equal(pstor.ild_lock_agro_lqk_na_vushky, "ph_door@open", "and the section it led to is remembered")
    -- Closing and opening the unlocked hatch by hand is not a lock being opened.
    action.st.locked, action.st.outcome = false, "ph_door@close"
    env.ph_door.action_door.use_callback(action, hatch, env.db.actor)
    equal(pstor.ild_lock_agro_lqk_na_vushky, "ph_door@open", "a plain toggle changes nothing remembered")
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
    equal(pstor.ild_lock_gar_tainik_s_artami, "nil", "a released box is remembered as released")
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
    equal(pstor.ild_lock_dom_prizrak_pechka, nil, "a section that times out is not remembered")

    -- The car hood taken apart with a tool ends in the never-again idiom and stays taken apart.
    local hood_ini = ini({["ph_idle:on_use"] = "{=actor_has_item(instrymentu_tonk)} %=give_items_to_actor(x)% ph_idle2",
        ["ph_idle2:on_use"] = "{+aiaiaiai}"})
    local hood = object(80, "agr_zapor_kapot")
    env.db.storage[80] = {active_section = "ph_idle"}
    use(env.ph_idle.action_idle, hood, {ini = hood_ini, outcome = "ph_idle2"})
    equal(pstor.ild_lock_agr_zapor_kapot, "ph_idle2", "a use that ends in the never-again idiom is remembered")

    -- A use that leads nowhere - the lockpick that snapped - records nothing.
    env.db.storage[81] = {active_section = "ph_idle"}
    local stuck = object(81, "stuck_box")
    use(env.ph_idle.action_idle, stuck, {ini = box_ini, outcome = "ph_idle"})
    equal(pstor.ild_lock_stuck_box, nil, "a failed attempt leaves nothing behind")
    equal(#calls.console, 0, "none of it touches the console")
end

print("PASS " .. tests .. " checks")
