local source_path = arg[1] or "payload/gamedata/scripts/ild_recipe_repairs.script"
local reference_root = arg[2]
local tests = 0

local function check(value, message)
    assert(value, message)
    tests = tests + 1
end

local function equal(actual, expected, message)
    check(actual == expected, message .. ": expected " .. tostring(expected) .. ", got " .. tostring(actual))
end

local recipes = {
    {module = "avtodroch", class = "slom_zaz_pochin", index = 1, parts = {
        item_zaz_benzbak = 1, item_akkym = 1, item_koleso_avto = 1, kanistra_benziny = 1,
        item_avto_fara = 1, item_avto_maslo = 1, item_podshipnik = 4, item_svecha_zajigania = 1
    }},
    {module = "avtodroch", class = "slom_zil_pochin", index = 1, parts = {
        item_benzbak_gryz = 2, item_akkym = 1, item_koleso_gryz = 4, kanistra_dizeli = 1,
        item_avto_fara = 1, item_avto_maslo = 1, item_podshipnik = 1, item_svecha_zajigania = 1
    }},
    {module = "avtodroch", class = "slom_niva_pochin", index = 1, parts = {
        item_podshipnik = 4, item_koleso_avto = 4, kanistra_benziny = 1,
        item_avto_fara = 2, item_avto_maslo = 1, item_avto_pryjinu = 4
    }},
    {module = "avtodroch", class = "slom_volga_pochin", index = 1, parts = {
        item_avto_xyinia10 = 1, item_avto_xyinia108 = 1, item_avto_xyinia107 = 1,
        kanistra_benziny = 1, item_avto_xyinia106 = 2, item_avto_xyinia101 = 1, item_akkym = 1
    }},
    {module = "avtodroch", class = "slom_jiglo1_pochin", index = 1, parts = {
        item_avto_xyinia108 = 1, item_avto_xyinia107 = 1, item_avto_xyinia106 = 1,
        item_avto_xyinia101 = 1, item_akkym = 1, item_avto_fara = 1, item_zaz_benzbak = 1,
        item_avto_maslo = 1, item_svecha_zajigania = 1
    }},
    {module = "avtodroch", class = "slom_jiglo2_pochin", index = 1, parts = {
        item_avto_xyinia105 = 1, item_avto_pryjinu = 2, kanistra_benziny = 1,
        item_avto_maslo = 1, item_svecha_zajigania = 1
    }},
    {module = "stanok", class = "sborka", index = 1, parts = {
        lr300_zapchasti1 = 1, lr300_zapchasti2 = 1, lr300_zapchasti3 = 1,
        lr300_zapchasti4 = 1, lr300_zapchasti5 = 1
    }},
    {module = "stanok", class = "sborka", index = 2, parts = {
        -- The two parts the mod's own bench asks for by a name no item carries; the recipe now spells them
        -- the way unique_items.ltx does, so the Vintorez button can actually be earned.
        vntrz_zapchast1 = 1, vntrz_zapchast2 = 1, vntrz_zapchasti3 = 1, wpn_addon_scope = 1
    }},
    {module = "stanok", class = "sborka", index = 3, parts = {
        svd_zapchasti1 = 1, svd_zapchasti2 = 1, svd_zapchasti3 = 1, svd_zapchasti4 = 1, wpn_addon_scope = 1
    }},
    {module = "stanok", class = "sborka", index = 4, parts = {
        vsk_zapchasti1 = 1, vsk_zapchasti2 = 1, vsk_zapchasti3 = 1, vsk_zapchasti4 = 1
    }}
}

local function fixture()
    local env = setmetatable({}, {__index = _G})
    env._G = env
    local calls = {init = 0, actions = 0, removed = {}, closed = 0, sound = 0, rewards = 0}
    local inventory, objects = {}, {}
    local actor = {
        object_count = function() return #inventory end,
        object = function(_, index) return inventory[index + 1] end
    }
    env.db = {actor = actor}
    env.alife = function() return {object = function(_, id) return objects[id] end} end
    env.avtodroch, env.stanok = {}, {}
    for _, recipe in ipairs(recipes) do
        local window_class = env[recipe.module][recipe.class] or {}
        env[recipe.module][recipe.class] = window_class
        window_class.InitControls = function(self, token)
            calls.init = calls.init + 1
            for index = 1, 4 do
                self["btn_" .. index] = {
                    enabled = self.original_enabled ~= false,
                    Enable = function(button, enabled) button.enabled = enabled end
                }
            end
            return token
        end
        window_class["btn" .. recipe.index] = function(self, first, second)
            calls.actions = calls.actions + 1
            calls.closed, calls.sound, calls.rewards = calls.closed + 1, calls.sound + 1, calls.rewards + 1
            for section, count in pairs(recipe.parts) do
                calls.removed[section] = (calls.removed[section] or 0) + count
                local remaining = count
                for index = #inventory, 1, -1 do
                    local item = inventory[index]
                    if remaining > 0 and item:section() == section and objects[item:id()] then
                        objects[item:id()] = nil
                        table.remove(inventory, index)
                        remaining = remaining - 1
                    end
                end
                assert(remaining == 0, "the original action must have enough server items")
            end
            return first, second
        end
    end
    env.avtodroch.slom_uaz_pochin = {InitControls = function() return "unmodified" end}
    env.stanok.pochinka2 = {rep_s1 = function() return "unmodified" end}
    local module = setmetatable({}, {__index = env})
    local chunk = assert(loadfile(source_path))
    setfenv(chunk, module)
    chunk()
    local next_id = 0
    local function add(section, count, no_server)
        for _ = 1, count do
            next_id = next_id + 1
            local id = next_id
            inventory[#inventory + 1] = {id = function() return id end, section = function() return section end}
            if not no_server then objects[id] = {id = id} end
        end
    end
    local function stock(recipe, deficit)
        for section, count in pairs(recipe.parts) do add(section, count - (section == deficit and 1 or 0)) end
    end
    local function window(recipe)
        return setmetatable({}, {__index = env[recipe.module][recipe.class]})
    end
    return env, calls, module, stock, window, add, inventory, objects
end

for _, recipe in ipairs(recipes) do
    local name = recipe.class .. ":btn" .. recipe.index
    for section in pairs(recipe.parts) do
        local _, calls, module, stock, window, _, inventory = fixture()
        module.install()
        stock(recipe, section)
        local dialog = window(recipe)
        equal(dialog:InitControls("init token"), "init token", name .. " keeps the initializer return")
        equal(dialog["btn_" .. recipe.index].enabled, false, name .. " rejects a deficit of " .. section)
        local before = #inventory
        equal(dialog["btn" .. recipe.index](dialog), nil, name .. " rejects a direct insufficient click")
        equal(calls.actions, 0, name .. " does not execute the original action on failure")
        equal(calls.closed, 0, name .. " does not close the dialog on failure")
        equal(calls.sound, 0, name .. " does not play success audio on failure")
        equal(calls.rewards, 0, name .. " does not award progress or a crafted item on failure")
        equal(next(calls.removed), nil, name .. " does not start partial ingredient removal")
        equal(#inventory, before, name .. " preserves all inventory on failure")
    end

    do
        local _, calls, module, stock, window, add, inventory = fixture()
        module.install()
        stock(recipe)
        add("unrelated", 1)
        local dialog = window(recipe)
        dialog:InitControls()
        equal(dialog["btn_" .. recipe.index].enabled, true, name .. " accepts exact quantities")
        local first, second = dialog["btn" .. recipe.index](dialog, "first", "second")
        equal(first, "first", name .. " forwards the first argument and return")
        equal(second, "second", name .. " forwards all argument and return values")
        equal(calls.actions, 1, name .. " executes the original action exactly once")
        for section, count in pairs(recipe.parts) do
            equal(calls.removed[section], count, name .. " preserves the cost of " .. section)
        end
        equal(#inventory, 1, name .. " consumes all required items including index zero")
        equal(inventory[1]:section(), "unrelated", name .. " preserves unrelated items")
        dialog["btn" .. recipe.index](dialog)
        equal(calls.actions, 1, name .. " a second click cannot reuse consumed ingredients")
    end

    do
        local _, calls, module, stock, window, _, inventory, objects = fixture()
        module.install()
        stock(recipe)
        local dialog = window(recipe)
        dialog:InitControls()
        equal(dialog["btn_" .. recipe.index].enabled, true, name .. " starts affordable")
        objects[inventory[1]:id()] = nil
        dialog["btn" .. recipe.index](dialog)
        equal(calls.actions, 0, name .. " rechecks server quantities after the dialog opens")
        equal(dialog["btn_" .. recipe.index].enabled, false, name .. " disables a stale affordable button")
        equal(next(calls.removed), nil, name .. " stale inventory cannot trigger partial payment")
    end
end

do
    local env, calls, module, stock, window, add, inventory = fixture()
    module.install()
    local recipe = recipes[1]
    stock(recipe, "item_podshipnik")
    add("item_podshipnik", 1, true)
    local dialog = window(recipe)
    dialog:InitControls()
    equal(dialog.btn_1.enabled, false, "client-only items do not count as payable ingredients")
    dialog:btn1()
    equal(calls.actions, 0, "missing server object cannot be used for payment")
    table.remove(inventory)
    for _, item in ipairs(inventory) do
        if item:section() == "item_podshipnik" then
            inventory[#inventory + 1] = item
            break
        end
    end
    dialog:btn1()
    equal(calls.actions, 0, "duplicate references cannot count the same server object twice")
    env.db.actor = nil
    dialog:btn1()
    equal(calls.actions, 0, "missing actor rejects the action")
end

do
    local env, calls, module, stock, window = fixture()
    module.install()
    stock(recipes[1])
    local dialog = window(recipes[1])
    dialog.original_enabled = false
    dialog:InitControls()
    equal(dialog.btn_1.enabled, false, "preflight never overrides an original disabled state")
    env.alife = function() return nil end
    dialog:btn1()
    equal(calls.actions, 0, "missing simulator rejects the action")
    equal(next(calls.removed), nil, "missing simulator leaves ingredients untouched")
    equal(env.avtodroch.slom_uaz_pochin.InitControls(), "unmodified", "unfinished UAZ UI remains untouched")
    equal(env.stanok.pochinka2.rep_s1(), "unmodified", "non-recipe repair mechanics remain untouched")
    local init, action = env.avtodroch.slom_zaz_pochin.InitControls, env.avtodroch.slom_zaz_pochin.btn1
    module.install()
    equal(env.avtodroch.slom_zaz_pochin.InitControls, init, "initializer wrapping is idempotent")
    equal(env.avtodroch.slom_zaz_pochin.btn1, action, "action wrapping is idempotent")
end

if reference_root then
    local sources = {}
    for _, name in ipairs({"avtodroch", "stanok"}) do
        local file = assert(io.open(reference_root .. "/gamedata/scripts/" .. name .. ".script", "rb"))
        sources[name] = file:read("*a"):gsub("\r\n", "\n")
        file:close()
    end
    for _, recipe in ipairs(recipes) do
        local name = recipe.class .. ":btn" .. recipe.index
        local body = assert(sources[recipe.module]:match("function " .. name .. "%(%)(.-)\nend"), name)
        local actual, count = {}, 0
        for section, amount in body:gmatch('amk%.remove_items%(%s*"([^"]+)"%s*,%s*(%d+)%s*%)') do
            actual[section] = (actual[section] or 0) + tonumber(amount)
            count = count + 1
        end
        local _, call_count = body:gsub("amk%.remove_items", "")
        equal(count, call_count, name .. " accounts for every actual removal call")
        for section, amount in pairs(actual) do
            equal(recipe.parts[section], amount, name .. " matches original removal of " .. section)
        end
        for section, amount in pairs(recipe.parts) do
            equal(actual[section], amount, name .. " adds no unverified cost for " .. section)
        end
    end

    local function original_fixture()
        local env, calls, module, stock, window, _, inventory, objects = fixture()
        env.CUIScriptWnd = {
            Init = function() end,
            Register = function() end,
            AddCallback = function(self, name, event, handler, context)
                self.callbacks = self.callbacks or {}
                self.callbacks[name] = {handler = handler, context = context}
            end,
            GetHolder = function()
                return {start_stop_menu = function() calls.closed = calls.closed + 1 end}
            end
        }
        env.class = function(name)
            local namespace = getfenv(2)
            return function(base) namespace[name] = setmetatable({}, {__index = base}) end
        end
        env.CScriptXmlInit = function()
            return {
                ParseFile = function() end,
                InitStatic = function() end,
                Init3tButton = function()
                    return {Enable = function(self, enabled) self.enabled = enabled end}
                end
            }
        end
        env.ui_events = {BUTTON_CLICKED = 1}
        env.sound_object = setmetatable({s2d = 1}, {
            __call = function() return {play = function() calls.sound = calls.sound + 1 end} end
        })
        local actor = env.db.actor
        actor.object = function(_, key)
            if type(key) == "number" then return inventory[key + 1] end
            for _, item in ipairs(inventory) do
                if item:section() == key then return item end
            end
        end
        actor.give_info_portion = function() calls.rewards = calls.rewards + 1 end
        actor.position = function() return {} end
        actor.level_vertex_id, actor.game_vertex_id, actor.id = function() return 1 end,
            function() return 1 end, function() return 1 end
        env.alife = function()
            return {
                object = function(_, id) return objects[id] end,
                create = function() calls.rewards = calls.rewards + 1 end
            }
        end
        env.amk = {
            get_object_number = function(_, section)
                local count = 0
                for _, item in ipairs(inventory) do
                    if item:section() == section then count = count + 1 end
                end
                return count
            end,
            remove_items = function(section, count)
                calls.removed[section] = (calls.removed[section] or 0) + count
                for index = #inventory, 1, -1 do
                    local item = inventory[index]
                    if count > 0 and item:section() == section and objects[item:id()] then
                        objects[item:id()] = nil
                        table.remove(inventory, index)
                        count = count - 1
                    end
                end
                assert(count == 0, "original handler must have enough server items")
            end
        }
        for name, source in pairs(sources) do
            local namespace = setmetatable({}, {__index = env})
            env[name] = namespace
            local chunk = assert(loadstring(source, "@reference/" .. name .. ".script"))
            setfenv(chunk, namespace)
            chunk()
        end
        module.install()
        return calls, stock, window, inventory
    end

    for _, recipe in ipairs(recipes) do
        local name = "original " .. recipe.class .. ":btn" .. recipe.index
        local calls, stock, window, inventory = original_fixture()
        stock(recipe)
        local dialog = window(recipe)
        dialog:InitControls()
        dialog:InitCallBacks()
        equal(dialog["btn_" .. recipe.index].enabled, true, name .. " exact quantities enable original UI")
        local callback = dialog.callbacks["btn_" .. recipe.index]
        equal(callback.handler, dialog["btn" .. recipe.index], name .. " registers the wrapped handler")
        callback.handler(callback.context)
        equal(calls.closed, 1, name .. " closes the original dialog once")
        equal(calls.rewards, 1, name .. " awards its original result once")
        check(calls.sound > 0, name .. " retains success audio")
        equal(#inventory, 0, name .. " pays its full original cost")
        for section, count in pairs(recipe.parts) do
            equal(calls.removed[section], count, name .. " consumes original count for " .. section)
        end

        calls, stock, window, inventory = original_fixture()
        stock(recipe, next(recipe.parts))
        dialog = window(recipe)
        dialog:InitControls()
        dialog:InitCallBacks()
        local before = #inventory
        equal(dialog["btn_" .. recipe.index].enabled, false, name .. " rejects an insufficient original UI")
        callback = dialog.callbacks["btn_" .. recipe.index]
        callback.handler(callback.context)
        equal(calls.closed, 0, name .. " failed original callback keeps the dialog open")
        equal(calls.rewards, 0, name .. " failed original callback awards nothing")
        equal(calls.sound, 0, name .. " failed original callback plays no success audio")
        equal(next(calls.removed), nil, name .. " failed original callback removes nothing")
        equal(#inventory, before, name .. " failed original callback preserves inventory")
    end
end

print("recipe_repairs_tests: " .. tests .. " checks passed")
