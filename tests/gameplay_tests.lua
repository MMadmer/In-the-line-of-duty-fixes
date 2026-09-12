local source_path = arg[1] or "payload/gamedata/scripts/ild_gameplay.script"
local tests = 0

local function check(value, message)
    assert(value, message)
    tests = tests + 1
end

local function equal(actual, expected, message)
    check(actual == expected, message .. ": expected " .. tostring(expected) .. ", got " .. tostring(actual))
end

local function fixture(with_qa)
    local calls = {subscribed = {}, added = {}, qa = 0, repairs = 0, mod_repairs = 0, quest_repairs = 0}
    local env = setmetatable({}, {__index = _G})
    env._G = env
    -- luabind's class "name" registers a callable table in the script's environment.
    env.class = function(name)
        env[name] = setmetatable({}, {__call = function(class_table, ...)
            local object = setmetatable({}, {__index = class_table})
            if class_table.__init then class_table.__init(object, ...) end
            return object
        end})
    end
    env.ild_script_repairs = {install = function() calls.repairs = calls.repairs + 1 end}
    env.ild_mod_repairs = {install = function() calls.mod_repairs = calls.mod_repairs + 1 end}
    env.ild_quest_repairs = {install = function() calls.quest_repairs = calls.quest_repairs + 1 end}
    if with_qa then env.ild_qa = {install = function() calls.qa = calls.qa + 1 end} end
    env.ph_door = {add_to_binder = function(object)
        calls.added[#calls.added + 1] = object
        return "door"
    end}
    env.xr_logic = {subscribe_action_for_events = function(object, storage, action)
        calls.subscribed[#calls.subscribed + 1] = {object = object, storage = storage, action = action}
    end}
    local module = setmetatable({}, {__index = env})
    local chunk = assert(loadfile(source_path))
    setfenv(chunk, module)
    chunk()
    return env, calls, module
end

local function make_object(name, cfg)
    return {
        tips = {}, usable = {},
        name = function() return name end,
        spawn_ini = function()
            if not cfg then return nil end
            return {
                line_exist = function(_, section, key) return section == "logic" and key == "cfg" end,
                r_string = function() return cfg end
            }
        end,
        set_tip_text = function(self, text) self.tips[#self.tips + 1] = text end,
        set_nonscript_usable = function(self, value) self.usable[#self.usable + 1] = value end
    }
end

do
    local env, calls, module = fixture(false)
    module.install()
    module.install()
    equal(calls.repairs, 1, "script repairs install once")
    equal(calls.mod_repairs, 1, "the forum-report repairs install once")
    equal(calls.quest_repairs, 1, "the quest repairs install once")
    equal(calls.qa, 0, "no QA probe runs when ild_qa is absent")
    local table_object = make_object("esc_tixona_xyinia4", "scripts\\door_logic.ltx")
    local storage = {}
    equal(env.ph_door.add_to_binder(table_object, "ini", "scheme", "section", storage), nil,
        "the decorative table skips the door binder")
    equal(#calls.added, 0, "the door binder is not called for the table")
    equal(#calls.subscribed, 1, "a decoration action is subscribed")
    equal(calls.subscribed[1].storage, storage, "the subscription uses the object's storage")
    calls.subscribed[1].action:reset_scheme()
    equal(table_object.tips[1], "", "the tip text is cleared")
    equal(table_object.usable[1], false, "the table is not usable")
    equal(env.ph_door.add_to_binder(make_object("esc_tixona_xyinia4", "scripts\\other.ltx"), "ini", "scheme",
        "section", {}), "door", "other configs keep the door logic")
    equal(env.ph_door.add_to_binder(make_object("real_door", "scripts\\door_logic.ltx"), "ini", "scheme",
        "section", {}), "door", "real doors keep the door logic")
    equal(env.ph_door.add_to_binder(make_object("esc_tixona_xyinia4", nil), "ini", "scheme", "section", {}),
        "door", "a missing spawn ini keeps the door logic")
    equal(#calls.added, 3, "the door binder ran for every other object")
end

do
    local _, calls, module = fixture(true)
    module.install()
    equal(calls.qa, 1, "the QA probe installs when ild_qa is present")
end

print("gameplay_tests: " .. tests .. " checks passed")
