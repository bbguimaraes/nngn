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

local function print_mat4(m)
    io.write("{")
    for r = 0, 3 do
        io.write("\n   ")
        for c = 0, 3 do
            io.write(" ", tostring(m[4 * c + r + 1]), ",")
        end
    end
    io.write("\n}\n")
end

local function get(t, ...)
    for _, k in ipairs{...} do
        t = t[k]
        if not t then
            break
        end
    end
    return t
end

local function deep_copy_values(x)
    if type(x) ~= "table" then
        return x
    end
    local ret = {}
    for k, v in pairs(x) do
        ret[k] = deep_copy_values(v)
    end
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

local function shift_table(v, t, d, inc, cmp)
    cmp = cmp or function(l, r) return l == r end
    for i, x in ipairs(t) do
        if cmp(x, v) then return t[shift(i, #t, inc, 1)] end
    end
    return d
end

local function find(t, x)
    for i, y in ipairs(t) do
        if x == y then
            return i
        end
    end
end

local function cmp(x, y) return not (y < x) end

local function lower_bound(t, x, f)
    f = f or cmp
    for i, y in ipairs(t) do
        if f(x, y) then
            return i
        end
    end
    return #t + 1
end

local function insert_sorted(t, x, f)
    local i <const> = lower_bound(t, x, f)
    table.insert(t, i, x)
    return i
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
    print_mat4 = print_mat4,
    get = get,
    deep_copy_values = deep_copy_values,
    map = map,
    fmt_time = fmt_time,
    fmt_size = fmt_size,
    shift = shift,
    shift_table = shift_table,
    find = find,
    lower_bound = lower_bound,
    insert_sorted = insert_sorted,
    get_double_tap_interval = get_double_tap_interval,
    set_double_tap_interval = set_double_tap_interval,
    check_double_tap = check_double_tap,
    reset_double_tap = reset_double_tap,
}
