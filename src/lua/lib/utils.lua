local function make_class(t, name, parent)
    t.__index = t
    t.__name = name
    return setmetatable(t, parent)
end

local Class <const> = {}

function Class.class(_, arg)
    if type(arg) ~= "string" then
        return make_class(arg)
    end
    return setmetatable({
        public = function(_, parent)
            return function(t) return make_class(t, arg, parent) end
        end,
    }, {
        __call = function(_, t) return make_class(t, arg) end,
    })
end

function Class:public(parent)
    return function(t) return make_class(t, nil, parent) end
end

setmetatable(Class, {__call = Class.class})

local function pprint(x, pre, write)
    write = write or io.write
    local t = type(x)
    if t ~= "table" then
        if pre then write(" ") end
        if t ~= "string" then return write(tostring(x), "\n") end
        return write(string.format("%q\n", x))
    end
    if pre then write("\n") else pre = "" end
    if next(x) == nil then return write("{}\n") end
    local npre = pre .. "  "
    for k, v in pairs(x) do
        write(pre, tostring(k), ":")
        pprint(v, npre, write)
    end
end

local function pformat(x)
    local ret = ""
    pprint(x, nil, function(...)
        for _, x in ipairs{...} do ret = ret .. tostring(x) end
    end)
    return ret
end

local function map(t, f, iter)
    local ret = {}
    for k, v in (iter or pairs)(t) do ret[k] = f(v) end
    return ret
end

local function fmt_time(ns, plus)
    plus = plus or ""
    local abs = math.abs(ns)
    local fmt = "%" .. plus .. "8.3f"
    if abs < 1e3 then return string.format(fmt .. "ns", ns)
    elseif abs < 1e6 then return string.format(fmt .. "µs", ns / 1e3)
    elseif abs < 1e9 then return string.format(fmt .. "ms", ns / 1e6)
    else return string.format(fmt .. "s", ns / 1e9) end
end

local function fmt_size(n)
    if n < (1 << 10) then return string.format("%.1f", n)
    elseif n < (1 << 20) then return string.format("%.1fK", n / (1 << 10))
    elseif n < (1 << 30) then return string.format("%.1fM", n / (1 << 20))
    else return string.format("%.1fG", n / (1 << 30)) end
end

local function shift(i, n, inc, base)
    if base then i = i - base end
    if inc then i = i + 1
    else i = i + n - 1 end
    i = i % n
    if base then i = i + base end
    return i
end

local function find(t, x)
    for i, y in ipairs(t) do
        if x == y then
            return i
        end
    end
end

local DOUBLE_TAP = {}
local DOUBLE_TAP_INTERVAL = 200

local function get_double_tap_interval() return DOUBLE_TAP_INTERVAL end
local function set_double_tap_interval(i) DOUBLE_TAP_INTERVAL = i end

local function check_double_tap(t, key, shift)
    if shift then key = key + 128 end
    local last = DOUBLE_TAP[key] or 0
    DOUBLE_TAP[key] = t
    return t - last <= DOUBLE_TAP_INTERVAL
end

local function reset_double_tap(key, shift)
    local t = DOUBLE_TAP
    if key then
        if shift then key = key + 128 end
        t = {key = true}
    end
    for k in pairs(t) do DOUBLE_TAP[k] = nil end
end

return {
    class = Class,
    pprint = pprint,
    pformat = pformat,
    map = map,
    fmt_time = fmt_time,
    fmt_size = fmt_size,
    shift = shift,
    find = find,
    get_double_tap_interval = get_double_tap_interval,
    set_double_tap_interval = set_double_tap_interval,
    check_double_tap = check_double_tap,
    reset_double_tap = reset_double_tap,
}
