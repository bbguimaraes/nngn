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

return {
    pprint = pprint,
    pformat = pformat,
    print_mat4 = print_mat4,
    get = get,
    deep_copy_values = deep_copy_values,
    map = map,
    fmt_time = fmt_time,
    fmt_size = fmt_size,
}
