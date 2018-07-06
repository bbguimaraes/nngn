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

return {
    pprint = pprint,
    pformat = pformat,
    map = map,
    fmt_time = fmt_time,
    fmt_size = fmt_size,
}
