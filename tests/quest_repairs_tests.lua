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
    env.level = {
        map_add_object_spot = function(id, kind, hint) calls.spots[#calls.spots + 1] = {id, kind, hint} end,
        map_remove_object_spot = function(id, kind) calls.spots[#calls.spots + 1] = {id, kind, false} end,
        map_has_object_spot = function() return 0 end,
        name = function() return "l01_escape" end,
        object_by_id = function(id) return objects[id] end
    }
    env.game = {translate_string = function(id) return id end}
    env.has_alife_info = function(name) return calls.infos[name] == true end
    env.xr_logic = {pstor_retrieve = function(_, key, default) return calls.infos[key] or default end,
        pstor_store = function(_, key, value) calls.infos[key] = value end}
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

print("PASS " .. tests .. " checks")
