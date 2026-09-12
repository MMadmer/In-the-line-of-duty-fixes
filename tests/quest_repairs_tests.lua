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

print("PASS " .. tests .. " checks")
