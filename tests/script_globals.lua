-- A module-level local is visible only below its declaration. A function above it that names it compiles to a
-- global lookup instead, which is nil at run time and, inside a pcall, fails silently for good. The compiler's own
-- listing is the only reliable view of which names a script reads as globals.
-- The engine compiles each script inside its own namespace header, which leaves 182 of Lua 5.1's 200 locals for the
-- script itself. Past that the game reports no compile error at all: the namespace resolves to nil and the first
-- script that touches it dies on the main menu. Desktop Lua loads the same file, so nothing else here would catch it.
local engine_local_budget = 182
local luac = assert(arg[1], "usage: script_globals.lua <luac> <script>...")
local checked, failures = 0, 0

for index = 2, #arg do
    local path = arg[index]
    local locals = {}
    local declared = 0
    for line in io.lines(path) do
        local names = string.match(line, "^local%s+function%s+([%w_]+)") or string.match(line, "^local%s+([%w_,%s]+)")
        for name in string.gmatch(names or "", "[%w_]+") do
            if not locals[name] then declared = declared + 1 end
            locals[name] = true
        end
    end
    if declared > engine_local_budget then
        failures = failures + 1
        io.stderr:write(path .. ": " .. declared .. " module locals, " .. engine_local_budget ..
            " is all the engine compiles; move new state inside the function that uses it\n")
    end
    -- cmd.exe strips one pair of quotes around the whole line, so the quoted program path needs an outer pair.
    -- A chunk that does not compile - Lua 5.1 refuses more than 200 active locals, for one - prints no listing.
    local listing = assert(io.popen('""' .. luac .. '" -p -l "' .. path .. '" 2>&1"'))
    local seen, compiled = {}, false
    for line in listing:lines() do
        compiled = compiled or string.find(line, "^main <") ~= nil
        if string.find(line, "luac", 1, true) and string.find(line, path, 1, true) then
            io.stderr:write(line .. "\n")
        end
        local name = string.match(line, "[GS]ETGLOBAL%s+[^;]*;%s*([%w_]+)")
        if name and locals[name] and not seen[name] then
            seen[name] = true
            failures = failures + 1
            io.stderr:write(path .. ": module local '" .. name .. "' is used as a global before its declaration\n")
        end
    end
    listing:close()
    if not compiled then
        failures = failures + 1
        io.stderr:write(path .. ": does not compile\n")
    end
    checked = checked + 1
end

assert(checked > 0, "no script was checked")
assert(failures == 0, failures .. " script problem(s)")
print("script_globals: " .. checked .. " scripts checked")
