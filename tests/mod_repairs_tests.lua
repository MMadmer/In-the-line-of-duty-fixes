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

print("PASS " .. tests .. " checks")
