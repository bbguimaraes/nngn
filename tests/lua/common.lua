local utils = require "nngn.lib.utils"

local function current_level()
    local ret = 3
    while true do
        if not debug.getinfo(ret, "f") then
            break
        end
        ret = ret + 1
    end
    return ret - 3
end

local function max_level()
    local env = os.getenv("NNGN_TEST_TRACE_LEVEL")
    if not env then return 0 end
    return env + 0
end

local function format_source(s)
    local first = string.sub(s, 0, 1)
    if first ~= "@" and first ~= "=" then
        return s
    end
    return string.sub(s, 2)
end

local function format_line(l)
    if l == -1 then return "" end
    return ":" .. l
end

local function format_level(l)
    local t = debug.getinfo(l, "Sn")
    return format_source(t.source)
        .. format_line(t.linedefined)
        .. ":" .. (t.name or "?")
end

local function setup_hook(cur_level)
    local level = cur_level + current_level()
    local max = max_level()
    debug.sethook(function(e)
        if e == "call" then
            level = level + 1
            if level > max then return end
            for i = 1, level do io.write("  ") end
            print(format_level(3))
        elseif e == "return" then
            level = level - 1
        end
    end, "cr")
end

local function fail(msg)
    if not os.getenv("NNGN_DEBUG_ON_FAILURE") then error(msg) end
    print(msg)
    debug.debug()
end

local function default_cmp(x, y) return x == y end

local function deep_cmp(x, y)
    if type(x) ~= "table" and type(y) ~= "table" then return x == y end
    if #x ~= #y then return false end
    for k in pairs(x) do
        if not deep_cmp(x[k], y[k]) then return false end
    end
    return true
end

local function assert_eq(actual, expected, cmp)
    local function append(s, ...)
        for _, x in ipairs({...}) do s = s .. x end
        return s
    end
    if not (cmp or default_cmp)(actual, expected) then
        fail(string.format(
            "want:\n%sgot:\n%s",
            utils.pformat(expected), utils.pformat(actual)))
    end
end

local function wrap_stdout(f)
    local stdout = io.output()
    local tmp = io.tmpfile()
    io.output(tmp)
    local ok, err = pcall(f)
    if not ok then tmp:close() error(err) end
    tmp:seek("set")
    local ret = tmp:read("a")
    tmp:close()
    io.output(stdout)
    return ret
end

return {
    setup_hook = setup_hook,
    fail = fail,
    deep_cmp = deep_cmp,
    assert_eq = assert_eq,
    wrap_stdout = wrap_stdout,
}
